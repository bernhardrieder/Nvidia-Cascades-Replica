struct v2pConnector
{
    float4 posH : SV_Position; //homogeneous clipspace
    float3 posW : POSITION; //worldspace coord
    float4 color : COLOR;
    float3 normalW : NORMAL;
};

cbuffer PerFrame : register(b0)
{
    float4x4 g_view;
    float4x4 g_viewProj;
    float4 g_sunLightDirection;
    float3 g_worldEyePosition;
    float g_appTime;
    float3 g_screenSize;
    float g_deltaTime;
}

Texture2D lichen1 : register(t0);
Texture2D lichen2 : register(t1);
Texture2D lichen3 : register(t2);

SamplerState LinearRepeatAnsio : register(s0);

#define MASTER_TEX_SCALE 1.45
#define texture_scale 0.2

#include "inoise.hlsli"

float4 main(v2pConnector v2p) : SV_Target
{
    // BLEND WEIGHTS FOR TRI-PLANAR PROJECTION
    //----------------------------------------------------------
    float3 blend_weights = abs(normalize(v2p.normalW)) - 0.2f;
    blend_weights *= 7;
    blend_weights = pow(blend_weights, 3);
    blend_weights = max(0, blend_weights);
    blend_weights /= dot(blend_weights, float3(1.f, 1.f, 1.f));
    //blend_weights /= (blend_weights.x + blend_weights.y + blend_weights.z).xxx;
    
    //----------------------- TRI-PLANAR PROJECTION ---------------------------
    float4 blendedColor;
    float3 PlanarTexScales = float3(1.f, 1.f, 1.f);
    //float2 coord1 = v2p.posW.yz * 1.5 * PlanarTexScales.x * texture_scale * MASTER_TEX_SCALE;
    //float2 coord2 = v2p.posW.zx * 1.5 * PlanarTexScales.y * texture_scale * MASTER_TEX_SCALE;
    //float2 coord3 = v2p.posW.xy * 1.5 * PlanarTexScales.z * texture_scale * MASTER_TEX_SCALE;
    float2 coord1 = v2p.posW.yz * texture_scale;
    float2 coord2 = v2p.posW.xz * texture_scale;
    float2 coord3 = v2p.posW.xy  * texture_scale;

    float4 mossCol1 = lichen1.Sample(LinearRepeatAnsio, coord1); //.yxz;
    float4 mossCol2 = lichen2.Sample(LinearRepeatAnsio, coord2); //.yxz;
    float4 mossCol3 = lichen3.Sample(LinearRepeatAnsio, coord3); //.yxz;
    //float4 mossCol1 = float4(0.f, 0.f, 0.f, 0.f);
    //float4 mossCol2 = float4(0.f, 0.f, 0.f, 0.f);
    //float4 mossCol3 = float4(0.f, 0.f, 0.f, 0.f);

    blendedColor =  mossCol1.xyzw * blend_weights.xxxx +
                    mossCol2.xyzw * blend_weights.yyyy +
                    mossCol3.xyzw * blend_weights.zzzz ;

    //blendedColor =  mossCol1 * float4(blend_weights.xxx, 1.f) +
    //                mossCol2 * float4(blend_weights.yyy, 1.f) +
    //                mossCol3 * float4(blend_weights.zzz, 1.f);

    //// --------------------- LO-FREQ COLOR NOISE -------------------
    //// add low-frequency colorization from noise:
    //{
    //    const float spread = 0.10; //0.09;
    //    moss_color.xyz *= (1 - spread) + (spread * 2) * float3(inoise(moss_color.xxx), inoise(moss_color.yyy), inoise(moss_color.zzz));
    //}

    return blendedColor;

    v2p.color = blendedColor;
    //simple lightning function
    //----------------------------------------------------------
    float4 lightIntensity = dot(g_sunLightDirection.xyz, v2p.normalW);
    float4 color = saturate(lightIntensity * v2p.color);
    float4 ambient = (0.1f * v2p.color);
    color = normalize(float4(color.xyz, 1) + ambient);
    return color;
}