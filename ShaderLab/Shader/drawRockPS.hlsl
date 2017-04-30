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

float4 main(v2pConnector v2p) : SV_Target
{
    //simple lightning function
    float4 lightIntensity = dot(g_sunLightDirection.xyz, v2p.normalW) * 100;
    float4 color = saturate(lightIntensity * v2p.color);
    float4 ambient = (0.1f * v2p.color);
    color = normalize(float4(color.xyz, 1) + ambient);
    return color;
}