#include "FireParticle.hlsli"

struct g2pConnector
{
    float4 PosH : SV_Position;
    float4 Color : COLOR;
    float2 Tex : TEXCOORD;
};

float4 main(g2pConnector g2p) : SV_TARGET
{
    return gTex.Sample(samLinear, g2p.Tex) * g2p.Color;
}