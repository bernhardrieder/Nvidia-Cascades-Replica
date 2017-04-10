#include "ShaderLab.hlsli"

float4 main(GSOutput IN) : SV_TARGET
{
    //IN.color.xyz = 0.f;
    return IN.color;
}