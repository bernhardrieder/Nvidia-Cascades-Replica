struct a2vConnector
{
    float4 wsPosition : POSITION;
    float3 wsNormal : NORMAL;
};

struct v2pConnector
{
    float4 pos : SV_Position;
    float4 color : COLOR;
    float3 normal : NORMAL;
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

v2pConnector main(a2vConnector a2v)
{
    v2pConnector v2p;

    matrix mvp = mul(projectionMatrix, mul(viewMatrix, worldMatrix));
    v2p.pos = mul(mvp, float4(a2v.wsPosition.xyz, 1.f));
    v2p.color = float4(1.f, 0.f, 0.f, 1.0f);
    v2p.normal = a2v.wsNormal;

    return v2p;
}