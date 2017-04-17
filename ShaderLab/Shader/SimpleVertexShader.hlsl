struct a2vConnector
{
    float4 wsCoord_Ambo : POSITION;
    float3 wsNormal : NORMAL;
};

struct v2pConnector
{
    float4 pos : SV_Position;
    float4 color : COLOR;
    float3 wsNormal : NORMAL;
};

cbuffer PerApplication : register(b0)
{
    matrix projectionMatrix;
}

cbuffer PerFrame : register(b1)
{
    matrix viewMatrix;
}

cbuffer PerObject : register(b2)
{
    matrix worldMatrix;
}

v2pConnector main(a2vConnector IN)
{
    v2pConnector OUT;

    matrix mvp = mul(projectionMatrix, mul(viewMatrix, worldMatrix));
    OUT.pos = mul(mvp, float4(IN.wsCoord_Ambo.xyz, 1.f));
    OUT.color = float4(1.f, 0.f, 0.f, 1.0f);
    OUT.wsNormal = IN.wsNormal;

    return OUT;
}