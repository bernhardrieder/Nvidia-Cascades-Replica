struct v2pConnector
{
    float4 pos : SV_Position;
    float4 color : COLOR;
    float3 normal : NORMAL;
};

float4 main(v2pConnector v2p) : SV_Target
{
    return float4(v2p.color.xyz, 1.0f);
}