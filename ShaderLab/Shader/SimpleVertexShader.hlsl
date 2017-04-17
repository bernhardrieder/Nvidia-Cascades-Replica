#include "ShaderLab.hlsli"

struct a2vConnector
{
    float4 wsCoord_Ambo : POSITION;
    float3 wsNormal : NORMAL;
};

struct v2gConnector
{
    float4 color : COLOR;
    float4 pos : POSITION1;
    float4 originalPos : POSITION;
};

struct g2pConnector
{
    float4 pos : SV_Position;
    float4 color : COLOR;
    float3 wsNormal : NORMAL;
};

g2pConnector main(a2vConnector IN)
{
    g2pConnector OUT;

    matrix mvp = mul(projectionMatrix, mul(viewMatrix, worldMatrix));
    OUT.pos = mul(mvp, float4(IN.wsCoord_Ambo.xyz, 1.f));
    //OUT.pos = float4(IN.wsCoord_Ambo.xyz, 0);
    //OUT.originalPos = IN.wsCoord_Ambo;
    //OUT.color = float4(IN.color, 1.0f);

    //OUT.pos = IN.wsCoord_Ambo;
    OUT.color = float4(1.f, 0.f, 0.f, 1.0f);
    OUT.wsNormal = IN.wsNormal;

    return OUT;
}