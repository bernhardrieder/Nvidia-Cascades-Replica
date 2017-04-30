struct a2vConnector
{
    float4 wsPosition : POSITION; //local position
    float3 wsNormal : NORMAL; //local normal
};

struct v2pConnector
{
    float4 posH : SV_Position; //homogeneous clipspace
    float3 posW : POSITION; //worldspace coord
    float4 color : COLOR;
    float3 normalW : NORMAL;
};

cbuffer PerApplication : register(b0)
{
    matrix projectionMatrix;
}

cbuffer PerFrame : register(b1)
{
    matrix g_view;
    float4x4 g_viewProj;
    float4 g_sunLightDirection;
    float3 g_worldEyePosition;
    float g_appTime;
    float3 g_screenSize;
    float g_deltaTime;
}

cbuffer PerObject : register(b2)
{
    matrix worldMatrix;
}

v2pConnector main(a2vConnector a2v)
{
    v2pConnector v2p;

    matrix worldViewProj = mul(projectionMatrix, mul(g_view, worldMatrix));
    //matrix worldInvTranspose = transpose(invert(worldMatrix));
    

    //transform to homogeneous clip space
    v2p.posH = mul(worldViewProj, float4(a2v.wsPosition.xyz, 1.f));

    //transform to world space
    v2p.posW = mul(worldMatrix, float4(a2v.wsPosition.xyz, 1.f)).xyz;
    v2p.normalW = a2v.wsNormal;
    
    v2p.color = float4(1.f, 0.f, 0.f, 1.0f);

    return v2p;
}