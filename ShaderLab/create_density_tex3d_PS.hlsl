
struct g2pConnector
{
    float4 rsCoord : SV_POSITION;
    float4 pos : TEXCOORD;
    uint index : SV_RenderTargetArrayIndex;
};

float main(g2pConnector input) : SV_TARGET0
{    
    if (input.index == 0)
        return 0.1f;
    else
        return 0.5f;
}