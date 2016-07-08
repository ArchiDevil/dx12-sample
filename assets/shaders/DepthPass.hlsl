// COPYRIGHT :)

cbuffer ModelParams : register(b0)
{
    float4x4 worldMatrix;
};

cbuffer FrameParams : register(b1)
{
    float4x4 viewProjectionMatrix;
    float4   eyePos;
};

struct VS_IN
{
    float3 position : POSITION;
#ifdef UseTessellation
    float3 normal   : NORMAL;
    float3 binormal : BINORMAL;
    float3 tangent  : TANGENT;
    float2 uv       : TEXCOORD;
#endif
};

struct VS_OUT
{
#ifdef UseTessellation
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD;
#else
    float4 position : SV_POSITION;
#endif
};

VS_OUT vs_main(VS_IN input)
{
    VS_OUT output;
#ifdef UseTessellation
    output.position = input.position;
    output.normal = mul(input.normal, (float3x3)worldMatrix);
    output.uv = input.uv;
#else
    output.position = mul(float4(input.position, 1.0f), worldMatrix);
    output.position = mul(output.position, viewProjectionMatrix);
#endif
    return output;
}

#ifdef UseTessellation
// Output control point
struct HS_OUT
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD;
};

// Output patch constant data.
struct HS_CONSTANT_DATA_OUTPUT
{
    float edges[3]  : SV_TessFactor;
    float inside : SV_InsideTessFactor;
};

#define MAX_POINTS 3

// Patch Constant Function
HS_CONSTANT_DATA_OUTPUT HSConstant(InputPatch<VS_OUT, MAX_POINTS> ip,
                                   uint PatchID : SV_PrimitiveID)
{
    float4 projectedPosition = mul(mul(float4(ip[0].position, 1.0f), worldMatrix), viewProjectionMatrix);
    float d = (1.0f - projectedPosition.z / projectedPosition.w) * 250.0f;
    d = clamp(d, 1.0, 15.0);

    HS_CONSTANT_DATA_OUTPUT output;
    output.edges[0] = d;
    output.edges[1] = d;
    output.edges[2] = d;
    output.inside = d / 1.5;
    return output;
}

[domain("tri")]                     // tri, quad, isoline
[partitioning("fractional_even")]   // integer, fractional_even, fractional_odd, pow2
[outputtopology("triangle_cw")]     // triangle_cw, triangle_ccw
[outputcontrolpoints(3)]
[patchconstantfunc("HSConstant")]
HS_OUT hs_main(InputPatch<VS_OUT, MAX_POINTS> ip,
               uint i : SV_OutputControlPointID,
               uint PatchID : SV_PrimitiveID)
{
    HS_OUT output;
    output.position = ip[i].position;
    output.normal   = ip[i].normal;
    output.uv       = ip[i].uv;
    return output;
}

//-----------------------------------------------------------------------------

struct DS_OUT
{
    float4 position : SV_POSITION;
};

float surfaceHeight(float2 uv)
{
    return (pow((sin((uv.x + 0.8) * 6.28) + 1.0) / 2.0, 4.0) *
            pow((cos((uv.y + 0.8) * 6.28) + 1.0) / 2.0, 4.0)) * 0.2;
}

[domain("tri")]
DS_OUT ds_main(HS_CONSTANT_DATA_OUTPUT input,
               float3 uvwCoord : SV_DomainLocation,
               const OutputPatch<HS_OUT, 3> patch)
{
    DS_OUT output;

    // Determine the position of the new vertex.
    float3 vertexPosition = patch[0].position * uvwCoord.x
        + patch[1].position * uvwCoord.y
        + patch[2].position * uvwCoord.z;

    float2 uvPosition = patch[0].uv * uvwCoord.x
        + patch[1].uv * uvwCoord.y
        + patch[2].uv * uvwCoord.z;

    float3 normalValue = patch[0].normal * uvwCoord.x
        + patch[1].normal * uvwCoord.y
        + patch[2].normal * uvwCoord.z;

    normalValue = normalize(normalValue);

    vertexPosition += normalValue * surfaceHeight(uvPosition);
    output.position = mul(float4(vertexPosition, 1.0f), worldMatrix);
    output.position = mul(output.position, viewProjectionMatrix);

    return output;
}
#endif

float ps_main(
#ifdef UseTessellation
    DS_OUT input
#else
    VS_OUT input
#endif
) : SV_TARGET
{
    return input.position.z;
}
