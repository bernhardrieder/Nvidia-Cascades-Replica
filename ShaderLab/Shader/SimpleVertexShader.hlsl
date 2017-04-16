#include "ShaderLab.hlsli"

struct a2vConnector
{
    float4 wsCoord_Ambo : POSITION;
    float3 wsNormal : NORMAL;
};

struct v2gConnector
{
    float4 color : COLOR;
    float4 pos : POSITIONT;
    float4 originalPos : POSITION;
};

v2gConnector main(a2vConnector IN)
{
    v2gConnector OUT;

    matrix mvp = mul(projectionMatrix, mul(viewMatrix, worldMatrix));
    OUT.pos = mul(mvp, float4(IN.wsCoord_Ambo));
    //OUT.pos = IN.wsCoord_Ambo;
    OUT.originalPos = IN.wsCoord_Ambo;
    //OUT.color = float4(IN.color, 1.0f);

    //OUT.pos = IN.wsCoord_Ambo;
    OUT.color = float4( 0.f, 0.f, 0.f, 1.0f );

    return OUT;
}