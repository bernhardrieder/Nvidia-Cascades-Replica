#include "FireParticle.hlsli"

struct g2pConnector
{
    float4 PosH : SV_Position;
    float4 Color : COLOR;
    float2 Tex : TEXCOORD;
    float DistanceToInitialPosition : DISTANCE;
};

float4 main(g2pConnector g2p) : SV_TARGET
{
    float4 color = gTex.Sample(samLinear, g2p.Tex) * g2p.Color;
    if(g2p.DistanceToInitialPosition > 1.5f)
        color.rgb = 0.1f;
    return color;
}