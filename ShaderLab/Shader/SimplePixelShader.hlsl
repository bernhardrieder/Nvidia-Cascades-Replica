struct g2pConnector
{
    float4 pos : SV_Position;
    float4 color : COLOR;
    float3 wsNormal : NORMAL;
};


float4 main(g2pConnector IN) : SV_Target
{
    //IN.color.xyz = 0.f;
    
    //return IN.color;
    return float4(1.f, 0.f, 0.f, 1.0f);
}