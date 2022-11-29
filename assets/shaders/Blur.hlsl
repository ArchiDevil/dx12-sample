// COPYRIGHT :)

Texture2D  Input  : register(t0);
Texture2D  Depth  : register(t1);
SamplerState texture_sampler : register(s0);

static const float PI = 3.1415926535897932384626433832795; //  pi
static const int   half_points = 7;
static const float d_diff_max = 0.005;

// Gaussian curve 
static float sigma = 2;

cbuffer BlurParams : register(b0)
{
    float2 step;
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

float gaus(float pos, float sigma_2pi_inv, float pow_sigma_2_inv)
{
    return sigma_2pi_inv * exp(pow_sigma_2_inv * pow(pos, 2));
}

// Gaus Blur
float ps_main(PS_IN input) : SV_TARGET
{
    float sigma_2pi_inv = 1.0 / (sigma * sqrt(2.0 * PI));
    float pow_sigma_2_inv = -1.0 / (2.0 * pow(sigma, 2));

    float w_sum = gaus(0, sigma_2pi_inv, pow_sigma_2_inv);;
    float val_gaus = w_sum * Input.Sample(texture_sampler, input.uv).x;
    float d_center = Depth.Sample(texture_sampler, input.uv).x;
    float points = 0;
    for (float pos = 0; pos < half_points; pos++)
    {
        float2 P_uv_l = input.uv + pos * step * 2;
        float2 P_uv_r = input.uv - pos * step * 2;

        float d_l = Depth.Sample(texture_sampler, P_uv_l).x;
        float d_r = Depth.Sample(texture_sampler, P_uv_r).x;

        float val_l = Input.Sample(texture_sampler, P_uv_l).x;
        float val_r = Input.Sample(texture_sampler, P_uv_r).x;

        float w = gaus(pos, sigma_2pi_inv, pow_sigma_2_inv);

        float d_diff_l = abs(d_center - d_l);
        float d_diff_r = abs(d_center - d_r);

        float factor_d_l = max(sign(d_diff_max - d_diff_l), 0);
        float factor_d_r = max(sign(d_diff_max - d_diff_r), 0);
        float factor_d = factor_d_l * factor_d_r;

        val_gaus += (val_l + val_r) * w * factor_d;
        w_sum += 2 * w * factor_d;
        points += 2 * factor_d;
    }

    // if we have only 1 point, do not calculate blur
    float factor = sign(points);
    w_sum = w_sum*factor + (1 - factor);
    val_gaus = val_gaus*factor + d_center*(1 - factor);

    return val_gaus * rcp(w_sum);
}
