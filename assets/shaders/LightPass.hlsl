// COPYRIGHT :)

Texture2D    diffuseTexture : register(t0);
Texture2D    normalTexture  : register(t1);
Texture2D    depthTexture   : register(t2);
Texture2D    shadowTexture  : register(t3);

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

float4 ps_main(PS_IN input) : SV_TARGET
{
    float4 diffuseSample = pow(diffuseTexture.Sample(texture_sampler, input.uv), 2.2);

    // restore world position of current pixel
    float4 worldPosition = restoreWorldPosition(input.uv);

    float3 lightPos = lightPosition.xyz;
    float3 lightDir = normalize(lightPos - worldPosition.xyz);

    float4 normalsSample = normalize(normalTexture.Sample(texture_sampler, input.uv));
    float diffuseFactor = max(dot(normalize(normalsSample.xyz), lightDir), 0.0f);

    // pixel distance calculation to compute fog
    float pixelDistance = distance(camPosition, worldPosition);

    float3 color = ambientColor * diffuseSample.xyz;
    float3 reflectedVector = reflect(-lightDir, normalsSample.xyz);
    // no pow here due to scene settings
    float specularFactor = pow(max(dot(normalize(camPosition.xyz - worldPosition.xyz), reflectedVector), 0.0f), 8.0f);

#ifdef ShadowMapping
    // project world position in shadow map coordinates [-1; 1]
    worldPosition = mul(worldPosition, shadowMatrix);
    worldPosition /= worldPosition.w;

    // convert projected coordinates into texture coordinates
    worldPosition = float4(worldPosition.x / 2.0f + 0.5f, -worldPosition.y / 2.0f + 0.5f, worldPosition.z, 0.0f);

    // get depth value from shadow map
    float shadowMapSample = shadowTexture.SampleCmpLevelZero(shadow_sampler, worldPosition.xy, worldPosition.z);
    float lightness = shadowMapSample;

    //const float pcfFilterSize = 3.0f;
    //const float halfPcfFilter = floor(pcfFilterSize / 2);
    //float lightness2 = 0.0f;

    //for (int i = -halfPcfFilter; i < halfPcfFilter + 1; ++i)
    //{
    //    for (int j = -halfPcfFilter; j < halfPcfFilter + 1; ++j)
    //    {
    //        if (shadowTexture.Sample(texture_sampler, float2(worldPosition.x + i / depthMapSize, worldPosition.y + j / depthMapSize)).r + 0.001f > worldPosition.z)
    //        {
    //            lightness2 += 1.0f;
    //        }
    //    }
    //}
    //lightness2 /= pcfFilterSize * pcfFilterSize;
    //lightness = (lightness + lightness2) / 2.0;
    
    // to light surface shadow coordinates should be more than visible
    color += diffuseSample.xyz * diffuseFactor * lightness + specularFactor * lightness * 100.0f;
#else
    color += diffuseSample.xyz * diffuseFactor + float3(specularFactor, specularFactor, specularFactor);
#endif

    float fogFactor = 1 - 1.0f / exp(pow(pixelDistance * (1.0f / (sceneSize * 4.0f)), 2.0f));
    color = lerp(color, fogColor, fogFactor);
    return float4(color, 1.0);
}
