struct v2pConnector
{
    float4 pos : SV_Position;
    float4 color : COLOR;
    float3 wsNormal : NORMAL;
};

float4 main(v2pConnector IN) : SV_Target
{
    return float4(1.f, 0.f, 0.f, 1.0f);
}