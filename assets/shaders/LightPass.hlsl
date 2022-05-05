// COPYRIGHT :)

Texture2D    diffuseTexture : register(t0);
Texture2D    normalTexture  : register(t1);
Texture2D    depthTexture   : register(t2);
Texture2D    shadowTexture  : register(t3);

TextureCube  reflectionTexture : register(t4);

SamplerState texture_sampler            : register(s0);
SamplerComparisonState shadow_sampler   : register(s1);

cbuffer SquadParams : register(b0)
{
    float4   lightPosition;

    float3   ambientColor;
    float    depthMapSize;

    float3   fogColor;
    float    sceneSize;

    float4x4 shadowMatrix;

    float4x4 inverseViewProj;

    float4   camPosition;
};

struct VS_IN
{
    float2 position : POSITION;
    float2 uv       : TEXCOORD;
};

struct PS_IN
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
};

PS_IN vs_main(VS_IN input)
{
    PS_IN output;
    output.position = float4(input.position, 0.0, 1.0);
    output.uv       = float2(input.uv);
    return output;
}

float4 restoreWorldPosition(float2 texcoord)
{
    // get depth value of the current pixel
    float depthSample = depthTexture.Sample(texture_sampler, texcoord).x;

    // convert texture coordinates into projected coordinates [0; 1] -> [-1; 1] and revert Y-axis
    float4 texelPosition = float4(2.0f * (texcoord.x - 0.5f), -2.0f * (texcoord.y - 0.5f), depthSample, 1.0f);

    // restore world position of current pixel
    float4 worldPosition = mul(texelPosition, inverseViewProj);
    worldPosition /= worldPosition.w;
    
    return worldPosition;
}

float F_Shlick(float NoV, float ior)
{
    float k = (1 - ior) / (1 + ior);
    float R0 = k * k;
    return R0 + (1 - R0) * pow(1 - NoV, 5.0);
}

float D_Beckmann(float roughness, float NoH)
{
    float m2 = roughness * roughness;
    float NoH2 = pow(NoH, 2.0);
    float NoH4 = NoH2 * NoH2;
    return 1.0 / (3.1415 * m2 * NoH4) * exp((NoH2 - 1) / (m2 * NoH4));
}

float G_Blinn(float NoH, float NoV, float VoH, float NoL, float HoL)
{
    float Gb = 2 * NoH * NoV / VoH;
    float Gc = 2 * NoH * NoL / HoL;
    return min(1.0, min(Gb, Gc));
}

float CookTorranceSpecular(float ior, float roughness, float NoV, float NoH, float NoL, float VoH, float HoL)
{
    return F_Shlick(NoV, ior) * D_Beckmann(roughness, NoH) * G_Blinn(NoH, NoV, VoH, NoL, HoL) / (3.1415 * NoL * NoV);
}

float4 ps_main(PS_IN input) : SV_TARGET
{
    float4 diffuseSample = pow(diffuseTexture.Sample(texture_sampler, input.uv), 2.2);

    // restore world position of the current pixel
    float4 worldPosition = restoreWorldPosition(input.uv);

    // we have to unpack normals from R11G11B10_FLOAT format because it does not store sign bit
    // so we just change [0; 1] range into [-1; 1]
    float3 N = normalize((normalTexture.Sample(texture_sampler, input.uv).xyz * 2.0) - 1.0);
    float3 V = normalize(camPosition.xyz - worldPosition.xyz);
    float3 L = normalize(lightPosition.xyz - worldPosition.xyz);
    float3 H = normalize(L + V);

    float diffuseFactor = max(dot(N, L), 0.0f);

    // simple reflection from cubemap
    float3 reflectionVector = normalize(reflect(V, N));
    float3 reflectionProbe = reflectionTexture.SampleLevel(texture_sampler, reflectionVector, 2).xyz;

    // pixel distance calculation to compute fog
    float pixelDistance = distance(camPosition, worldPosition);

    float NoV = dot(N, V);
    float NoH = dot(N, H);
    float NoL = dot(N, L);
    float VoH = dot(V, H);
    float HoL = dot(H, L);
    float specularFactor = CookTorranceSpecular(1.5f, 0.05f, NoV, NoH, NoL, VoH, HoL);
    float fresnel = F_Shlick(NoV, 1.5f);
    float3 color = ambientColor * diffuseSample.xyz + reflectionProbe * 0.5 * fresnel;

#ifdef ShadowMapping
    // project world position in shadow map coordinates [-1; 1]
    worldPosition = mul(worldPosition, shadowMatrix);
    worldPosition /= worldPosition.w;

    // convert projected coordinates into texture coordinates
    worldPosition = float4(worldPosition.x / 2.0f + 0.5f, -worldPosition.y / 2.0f + 0.5f, worldPosition.z, 0.0f);

    // get depth value from shadow map
    float shadowMapSample = shadowTexture.SampleCmpLevelZero(shadow_sampler, worldPosition.xy, worldPosition.z);
    float lightness = shadowMapSample;

#ifdef PCFFiltering
    const float pcfFilterSize = 3.0f;
    const float halfPcfFilter = floor(pcfFilterSize / 2);
    float lightness2 = 0.0f;

    for (int i = -halfPcfFilter; i < halfPcfFilter + 1; ++i)
    {
        for (int j = -halfPcfFilter; j < halfPcfFilter + 1; ++j)
        {
            if (shadowTexture.Sample(texture_sampler, float2(worldPosition.x + i / depthMapSize, worldPosition.y + j / depthMapSize)).r + 0.001f > worldPosition.z)
            {
                lightness2 += 1.0f;
            }
        }
    }
    lightness2 /= pcfFilterSize * pcfFilterSize;
    lightness = (lightness + lightness2) / 2.0;
#endif
    
    // to light surface shadow coordinates should be more than visible
    color += lightness * (diffuseSample.xyz * diffuseFactor * (1 - specularFactor) + specularFactor) * 1.5f;
#else
    color += (diffuseSample.xyz * diffuseFactor * (1 - specularFactor) + specularFactor) * 1.5f;
#endif

    float fogFactor = 1 - 1.0f / exp(pow(pixelDistance * (1.0f / (sceneSize * 4.0f)), 2.0f));
    color = lerp(color, fogColor, fogFactor);
    return float4(color, 1.0);
}
