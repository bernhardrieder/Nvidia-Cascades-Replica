struct v2pConnector
{
    float4 pos : SV_Position;
    float4 color : COLOR;
    float3 normal : NORMAL;
};

cbuffer PerFrame : register(b0)
{
    matrix viewMatrix; //unused in this shader
    float4 sunLightDirection;
}

float4 main(v2pConnector v2p) : SV_Target
{
    //simple lightning function
    float4 lightIntensity = dot(sunLightDirection.xyz, v2p.normal) * 100;
    float4 color = saturate(lightIntensity * v2p.color);
    float4 ambient = (0.1f * v2p.color);
    color = normalize(float4(color.xyz, 1) + ambient);
    return color;
}