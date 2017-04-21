
struct g2pConnector
{
    float4 pos : TEXCOORD;
    uint index : SV_RenderTargetArrayIndex;
    float4 rsCoord : SV_POSITION;
};

#include "inoise.hlsli"

float3 rot(float3 coord, float4x4 mat)
{
    return float3(dot(mat._11_12_13, coord), // 3x3 transform,
                 dot(mat._21_22_23, coord), // no translation
                 dot(mat._31_32_33, coord));
}

float2x2 rotate2d(float angle)
{
    return float2x2(cos(angle), -sin(angle), 
                    sin(angle), cos(angle));
}

static float2 pillars[4] =
{
    -0.8f, 0.8f,
    0.9f, -0.9f,
    0.8f, .8f,
    -0.7f, -0.2f,
};

float main(g2pConnector input) : SV_TARGET
{    
    float density = 0.f;

    //tweak input
    input.pos.xy *= 1.8f; // xy scaling
    input.pos.z *= 0.65f;

    //pillar (slide 12)
    for (int i = 0; i < 3; ++i)
    {
        density += 1 / length(input.pos.xy - mul(pillars[i], rotate2d(input.index * 0.1))) - 1-((turbulence(input.pos.xzy, 8, 2.23, 0.353) - 0.25) * 3);
    }
    
    //water flow channel (slide 14)
    density -= 1 / length(input.pos.xy) - ((turbulence(input.pos.xzy, 4, 4.417, 0.25) - 0.25) * 10);

    //keep solid rock "in bounds" (slide 15)
    density = density - pow(length(input.pos.xy), 3);

    //helix (slide 16)
    float2 vec = float2(cos(input.pos.z), sin(input.pos.z));
    density += dot(vec, input.pos.xy);

    //shelves (slide 17)
    density += cos(input.pos.z);

    //add some random noise
    float x = turbulence(input.pos.xyz, 1) * 5.56f;
    float4x4 rotation = float4x4(
                            float4(x, x, x, 0),
                            float4(x, x, x, 0),
                            float4(x, x, x, 0),
                            float4(0, 0, 0, 0));
    
    density += fBm(rot(input.pos.xyz, rotation), 4, 10.16, inoise(input.pos.xyz))*10;
    density += (turbulence(input.pos.xzy, 16, 4.417, 0.25) - 0.25) * 10;
    density += (inoise(input.pos.yzx) + inoise(input.pos.yxx) * inoise(input.pos.zxy)) * 10;

    return density;
}