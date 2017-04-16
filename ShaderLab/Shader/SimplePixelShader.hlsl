struct g2pConnector
{
    float4 color : COLOR;
    float4 pos : SV_Position;
};


float4 main(g2pConnector IN) : SV_Target
{
    //IN.color.xyz = 0.f;
    return IN.color;
}