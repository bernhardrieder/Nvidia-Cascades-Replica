struct a2vConnector
{
    float4 posL : POSITION; //local position
    float3 normalL : NORMAL0; //local vertex normal
    float3 SurfaceNormalL : NORMAL1; //local surface normal
};

struct v2pConnector
{
    float4 posH : SV_Position; //homogeneous clipspace
    float3 posW : POSITION; //worldspace coord
    float3 normalW : NORMAL0;
    float3 SurfaceNormalW : NORMAL1; //local surface normal
};

cbuffer PerApplication : register(b0)
{
    float4x4 g_projection;
    float g_discplacementDepth = 0.08f;
    float g_initialStepIterations = 10;
    float g_refinementStepIterations = 5;
}

cbuffer PerFrame : register(b1)
{
    float4x4 g_view;
    float4x4 g_viewProj;
    float4 g_sunLightDirection;
    float3 g_worldEyePosition;
    float g_appTime;
    float3 g_screenSize;
    float g_deltaTime;
}

cbuffer PerObject : register(b2)
{
    float4x4 g_world;
    float4x4 g_worldInvTranspose;
}

v2pConnector main(a2vConnector a2v)
{
    v2pConnector v2p;

    //float4x4 worldViewProj = mul(projectionMatrix, mul(g_view, worldMatrix));
    //float4x4 viewProj = mul(projectionMatrix, g_view);
    //matrix worldInvTranspose = transpose(invert(worldMatrix));
    
    //transform to world space
    v2p.posW = mul(g_world, float4(a2v.posL.xyz, 1.f)).xyz;

    v2p.normalW = mul(g_worldInvTranspose, float4(a2v.normalL.xyz, 1.f)).xyz;
    v2p.normalW = normalize(v2p.normalW);
    //v2p.normalW = a2v.normalL;

    v2p.SurfaceNormalW = mul(g_worldInvTranspose, float4(a2v.SurfaceNormalL.xyz, 1.f)).xyz;
    v2p.SurfaceNormalW = normalize(v2p.SurfaceNormalW);

    //transform to homogeneous clip space
    //v2p.posH = mul(worldViewProj, float4(a2v.wsPosition.xyz, 1.f));
    v2p.posH = mul(g_viewProj, float4(v2p.posW.xyz, 1.f));
    
    return v2p;
}