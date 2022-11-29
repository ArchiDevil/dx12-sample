// COPYRIGHT :)

Texture2D          depthTexture   : register(t0);
Texture2D<float2>  noiseTexture   : register(t1);

SamplerState texture_sampler            : register(s0);
SamplerState noise_sampler              : register(s1);

static const float PI = 3.1415926535897932384626433832795; //  pi

// limits
static const float h_limit_min = 0.4; // bias
static const float h_limit_max = 1.0; // edges
static const float AO_limit_min = 0.01; // bias
static const float noise_influence = 0.2;

// sampling directions
static const float rays_num = 8;
static const float ray_step = (2.0 * PI) / rays_num;
static const float rays_num_inv = rcp(rays_num);

// point position on a sampling direction
static const float point_pos_num = 5;
static const float point_pos_max = 25;
static const float point_pos_next_step = point_pos_max / point_pos_num;
static const float point_pos_next_step_inv = rcp(point_pos_max);

cbuffer FrameParams : register(b0)
{
    float4x4 viewProjectionInvMatrix;
    float2   InvScreenSize;
};

struct PS_IN
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
};

struct VS_IN
{
    float2 position : POSITION;
    float2 uv       : TEXCOORD;
};

PS_IN vs_main(VS_IN input)
{
    PS_IN output;
    output.position = float4(input.position, 0.0, 1.0);
    output.uv = float2(input.uv);
    return output;
}

// convert a vector to view space
float3 to_view_space(float3 p)
{
    float4 p1 = float4(p.x * 2.0 - 1.0, (1.0 - p.y) * 2.0 - 1.0, p.z, 1.0f);
    float4 p2 = mul(p1, viewProjectionInvMatrix);
    return p2.rgb * rcp(p2.a);
}

float attenuate(float r)
{
    float factor = max(sign(1.0 - r), 0);
    return (1.0 - r*r) * factor;
}

// HBAO
float ps_main(PS_IN input) : SV_TARGET
{
    float center_depth = depthTexture.Sample(texture_sampler, input.uv).x;

    // calc max H vector
    float H_max_l = length(to_view_space(float3(0, point_pos_max + point_pos_next_step, 0)));

    // center point
    float3 CP_vs = to_view_space(float3(input.uv, center_depth));

    // noise
    float raw_noise = (noiseTexture.Sample(noise_sampler, input.uv * 17 * log(length(CP_vs))).x - 0.5) * noise_influence;
    float ray_jitter = ray_step * raw_noise;

    // For each 2D direction
    float AO = 0;
    // float ray_angel = 0;
    for (float ray_angel = 0; ray_angel <= (2.0 * PI); ray_angel += ray_step)
    {
        float2 dir_clear = float2(cos(ray_angel), sin(ray_angel));

        float ray_angel_noised = ray_angel + ray_jitter;
        float2 dir_noised = float2(cos(ray_angel_noised), sin(ray_angel_noised));

        // Calculate tangent angle
        float2 sinlge_point_step = dir_clear * InvScreenSize;
        float2 T_uv_1 = input.uv - sinlge_point_step;
        float2 T_uv_2 = input.uv + sinlge_point_step;
        float3 TP_vs_1 = to_view_space(float3(T_uv_1, depthTexture.SampleLevel(texture_sampler, T_uv_1, 0).x));
        float3 TP_vs_2 = to_view_space(float3(T_uv_2, depthTexture.SampleLevel(texture_sampler, T_uv_2, 0).x));
        float3 T = TP_vs_2 - TP_vs_1;

        float t = -asin(T.z * rcp(length(T)));

        // Calculate max horisontal angle (h)
        float h_max = -10.0;
        float r_max = 0.0;
        float rayAO = 0;
        // float point_pos = point_pos_next_step;
        for (float point_pos = 0; point_pos < point_pos_max; point_pos += point_pos_next_step)
        {
            float2 shift = point_pos * InvScreenSize;
            float2 H_uv = input.uv + dir_clear * shift;
            float2 H_uv_noised = input.uv + dir_noised * shift;

            // new point
            float d = depthTexture.SampleLevel(texture_sampler, H_uv_noised, 0).x;

            float3 HP_vs = to_view_space(float3(H_uv, d));
            float3 H = HP_vs - CP_vs;
            float h_l = length(H);
            float h = -asin(H.z * rcp(h_l));

            float localAO = (sin(h) - sin(t));
            float factor_bias_AO = max(sign(localAO - AO_limit_min), 0);
            localAO *= factor_bias_AO;

            // if H exeeds maxinmal length, do not count such points
            // depth discontinuity
            float factor_h_l = max(sign(H_max_l - h_l), 0);
            localAO *= factor_h_l;

            rayAO = max(localAO, rayAO);

            // max angel and keep distance to the point with max angel
            float keep_r = max(sign(h_max - h), 0);
            r_max = max(r_max, (h_l * point_pos_next_step_inv) * keep_r);
            h_max = max(h_max, h);
        }

        float factor_edge = max(sign(h_limit_max - h_max), 0);
        float factor_bias_h = max(sign(h_max - h_limit_min), 0);

        AO += attenuate(r_max) * clamp(rayAO, 0.0, 1.0) * factor_bias_h * factor_edge;
    }

    // Calculate average AO
    AO *= rays_num_inv;
    return 1.0 - AO;
}
