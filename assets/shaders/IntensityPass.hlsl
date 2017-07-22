// COPYRIGHT :)

struct OutLuminanceBufferType
{
    float AverageLuminance;
    float MaxLuminance;
};

Texture2D<float4> ScreenQuad                                    : register(t0);
RWStructuredBuffer<OutLuminanceBufferType> LineIntensityBuffer  : register(u0);
RWStructuredBuffer<OutLuminanceBufferType> IntensityOut         : register(u1);

#define MAX_THREADS_IN_GROUP 32

[numthreads(MAX_THREADS_IN_GROUP, 1, 1)]
void cs_main(uint3 ThreadID : SV_GroupThreadID,
             uint3 GroupID : SV_GroupID)
{
    uint Width = 0;
    uint Height = 0;
    uint NumberOfLevels = 0;

    ScreenQuad.GetDimensions(0, // mip-level
                             Width,
                             Height,
                             NumberOfLevels);

    uint yCoord = GroupID.x * MAX_THREADS_IN_GROUP + ThreadID.x;

    if (yCoord < Height)
    {
        float LineLuminance = 0.0f;
        float MaxLineLuminance = 0.0f;
        for (uint j = 0; j < Width; ++j)
        {
            uint2 LoadCoords = uint2(j, yCoord);
            float3 TextureRGB = ScreenQuad[LoadCoords].rgb;
            float luminance = 0.2126 * TextureRGB.r + 0.7152 * TextureRGB.g + 0.0722 * TextureRGB.b;
            MaxLineLuminance = max(luminance, MaxLineLuminance);
            LineLuminance += log(0.001 + luminance); // delta + Y channel
        }
        LineIntensityBuffer[yCoord].AverageLuminance = LineLuminance;
        LineIntensityBuffer[yCoord].MaxLuminance = MaxLineLuminance;
    }

    DeviceMemoryBarrierWithGroupSync();

    // compute overall luminance
    if (ThreadID.x == 0 && GroupID.x == 0)
    {
        float OverallLuminance = 0.0f;
        float MaxImageLuminance = 0.0f;
        for (uint i = 0; i < Height; ++i)
        {
            OverallLuminance += LineIntensityBuffer[i].AverageLuminance;
            MaxImageLuminance = max(LineIntensityBuffer[i].MaxLuminance, MaxImageLuminance);
        }

        IntensityOut[0].AverageLuminance = exp(1.0 / (Width * Height) * OverallLuminance);
        // maximum luminance is clamped due to enforcing color burning with Reinhard-operator model
        IntensityOut[0].MaxLuminance = clamp(MaxImageLuminance, 0.4, 1.2);
    }
}
