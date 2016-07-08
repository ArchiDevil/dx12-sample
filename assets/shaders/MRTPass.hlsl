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

#ifdef RootConstants
float texCoordShift : register(b2);
#endif

#ifdef UseTextures
Texture2D    g_texture : register(t0);
SamplerState g_sampler : register(s0);
#endif

//-----------------------------------------------------------------------------

struct VS_IN
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float3 binormal : BINORMAL;
    float3 tangent  : TANGENT;
    float2 uv       : TEXCOORD;
};

struct VS_OUT
{
#ifdef UseTessellation
    float3 position : POSITION;
#else
    float4 position : SV_POSITION;
#endif
    float3 normal   : NORMAL;
    float3 binormal : BINORMAL;
    float3 tangent  : TANGENT;
    float2 uv       : TEXCOORD;
};

VS_OUT vs_main(VS_IN input)
{
    VS_OUT output;
#ifdef UseTessellation
    output.position = input.position;
#else
    output.position = mul(mul(float4(input.position, 1.0f), worldMatrix), viewProjectionMatrix);
#endif
    output.normal = mul(input.normal, (float3x3)worldMatrix);
    output.binormal = mul(input.binormal, (float3x3)worldMatrix);
    output.tangent = mul(input.tangent, (float3x3)worldMatrix);
#ifdef RootConstants
    output.uv = float2(input.uv.x + texCoordShift, input.uv.y + texCoordShift);
#else
    output.uv = input.uv;
#endif
    return output;
}

//-----------------------------------------------------------------------------

#ifdef UseTessellation
// Output control point
struct HS_OUT
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float3 binormal : BINORMAL;
    float3 tangent  : TANGENT;
    float2 uv       : TEXCOORD;
};

// Output patch constant data.
struct HS_CONSTANT_DATA_OUTPUT
{
    float edges[3]        : SV_TessFactor;
    float inside          : SV_InsideTessFactor;
};

#define MAX_POINTS 3

// Patch Constant Function
HS_CONSTANT_DATA_OUTPUT HSConstant(InputPatch<VS_OUT, MAX_POINTS> ip,
                                   uint PatchID : SV_PrimitiveID)
{
    float4 projectedPosition = mul(mul(float4(ip[0].position, 1.0), worldMatrix), viewProjectionMatrix);
    float d = (1.0 - projectedPosition.z / projectedPosition.w) * 250.0;
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
    output.normal = ip[i].normal;
    output.binormal = ip[i].binormal;
    output.tangent = ip[i].tangent;
    output.uv = ip[i].uv;
    return output;
}

//-----------------------------------------------------------------------------

struct DS_OUT
{
    float4 position : SV_POSITION;
    float3 normal   : NORMAL;
    float3 binormal : BINORMAL;
    float3 tangent  : TANGENT;
    float2 uv       : TEXCOORD;
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
    float3 vertexPosition = uvwCoord.x * patch[0].position + 
                            uvwCoord.y * patch[1].position + 
                            uvwCoord.z * patch[2].position;

    float2 uvPosition = patch[0].uv * uvwCoord.x + patch[1].uv * uvwCoord.y + patch[2].uv * uvwCoord.z;

    // new binormal computation
    float3 prePos = vertexPosition - patch[0].binormal * 0.01f + patch[0].normal * surfaceHeight(float2(uvPosition.x - 0.01f, uvPosition.y));
    float3 postPos = vertexPosition + patch[0].binormal * 0.01f + patch[0].normal * surfaceHeight(float2(uvPosition.x + 0.01f, uvPosition.y));
    float3 binormal = normalize(postPos - prePos);

    // new tangent computation
    prePos = vertexPosition - patch[0].tangent * 0.01f + patch[0].normal * surfaceHeight(float2(uvPosition.x, uvPosition.y - 0.01f));
    postPos = vertexPosition + patch[0].tangent * 0.01f + patch[0].normal * surfaceHeight(float2(uvPosition.x, uvPosition.y + 0.01f));
    float3 tangent = normalize(postPos - prePos);

    // new normal
    float3 normal = normalize(cross(binormal, tangent));

    vertexPosition += patch[0].normal * surfaceHeight(uvPosition);
    // Calculate the position of the new vertex against the world, view, and projection matrices.
    output.position = mul(float4(vertexPosition, 1.0f), worldMatrix);
    output.position = mul(output.position, viewProjectionMatrix);

    output.normal = normal;
    output.binormal = binormal;
    output.tangent = tangent;
    output.uv = uvPosition;

    return output;
}
#endif

//-----------------------------------------------------------------------------

struct PS_OUT
{
    float4 diffuse : SV_TARGET0;
    float4 normals : SV_TARGET1;
    float  depth   : SV_TARGET2;
};

PS_OUT ps_main(
#ifdef UseTessellation
    DS_OUT input
#else
    VS_OUT input
#endif
)
{
    PS_OUT output;
#ifdef UseTextures
    float4 diffuseSample = g_texture.Sample(g_sampler, input.uv);
#else
    float4 diffuseSample = float4(0.75f, 0.75f, 0.75f, 1.0f);
#endif
    output.diffuse = diffuseSample;
    output.normals = float4(normalize(input.normal), 1.0f);
    output.depth = input.position.z;
    return output;
}
