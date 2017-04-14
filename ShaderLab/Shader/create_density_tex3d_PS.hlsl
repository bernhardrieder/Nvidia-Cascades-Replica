
struct g2pConnector
{
    float4 pos : TEXCOORD;
    uint index : SV_RenderTargetArrayIndex;
    float4 rsCoord : SV_POSITION;
};

Texture2D g_noise1;
Texture2D g_noise2;
Texture2D g_noise3;
Texture2D g_noise4;
Texture2D g_noise5;
Texture2D g_noise6;
Texture2D g_noise7;
Texture2D g_noise8;
SamplerState g_samLinearWrap;

#include "inoise.hlsli"

float3 rot(float3 coord, float4x4 mat)
{
    return float3(dot(mat._11_12_13, coord), // 3x3 transform,
                 dot(mat._21_22_23, coord), // no translation
                 dot(mat._31_32_33, coord));
}

float2 rot(float2 coord, float2x2 mat)
{
    return float2(dot(mat._11_12, coord),
                  dot(mat._21_22, coord));
}
float2x2 rotate2d(float angle)
{
    return float2x2(cos(angle), -sin(angle), 
                    sin(angle), cos(angle));
}

static float2 pillars[3] =
{
    -0.2f, 0.5f,
    1.f, 0.9f,
    -0.6f, -.5f
};

float main(g2pConnector input) : SV_TARGET
{    
    float density = 0.f;
    //float distToCenter = distance(input.pos.xy, float2(0.f, 0.f));
    ///** filled circle */
    //if (distToCenter <= 0.1f)
    //    density = 0.f;

    ///** circle */
    //if (distToCenter <= 0.9f && distToCenter >= 0.85f)
    //    density = 0.f;

    //tweak input
    input.pos.xy *= 2.0f; // xy scaling
    input.pos.z *= 0.5f;

    //pillar (slide 12)
    for (int i = 0; i < 3; ++i)
    {
        density += 1 / length(input.pos.xy - pillars[i]) - 1;
    }
    
    //water flow channel (slide 14)
    density -= 1 / length(input.pos.xy) - 1;

    //keep solid rock "in bounds" (slide 15)
    density = density - pow(length(input.pos.xy), 3);

    //helix (slide 16)
    float2 vec = float2(cos(input.pos.z), sin(input.pos.z));
    density += dot(vec, input.pos.xy);

    //shelves (slide 17)
    density += cos(input.pos.z);

    float x = turbulence(input.pos.xyz,1) * 5.56f;
    //float4x4 rotation = float4x4(
    //                        float4(cos(x),-sin(x),0,0),
    //                        float4(sin(x), cos(x), 0, 0),
    //                        float4(0, 0, 1, 0),
    //                        float4(0, 0, 0, 0));
    float4x4 rotation = float4x4(
                            float4(x, x, x, 0),
                            float4(x, x, x, 0),
                            float4(x, x, x, 0),
                            float4(0, 0, 0, 0));
    
    density += fBm(rot(input.pos.xyz, rotation), 4, 10.16, inoise(input.pos.xyz))*5;

    return density;
}