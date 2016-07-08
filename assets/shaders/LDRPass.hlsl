// COPYRIGHT :)

Texture2D    colorTexture   : register(t0);
SamplerState textureSampler : register(s0);

cbuffer SquadParams : register(b0)
{
    float averageLuminance;
    float maxLuminance;
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

float4 ps_main(PS_IN input) : SV_TARGET
{
    float3 TextureRGB = colorTexture.Sample(textureSampler, input.uv).rgb;

    float pixelLuminance = 0.2126 * TextureRGB.r + 0.7152 * TextureRGB.g + 0.0722 * TextureRGB.b;
    float newLuminance = pixelLuminance * (1 + pixelLuminance / (maxLuminance * maxLuminance)) / (1 + pixelLuminance);
    float scale = newLuminance / pixelLuminance;
    float3 rgbColor = TextureRGB * scale;
    return float4(rgbColor, 1.0);
}
