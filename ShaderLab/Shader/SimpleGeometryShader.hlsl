#include "ShaderLab.hlsli"

//Texture3D g_densityTex;
//SamplerState g_samLinearWrap;

struct v2gConnector
{
    float4 color : COLOR;
    float4 pos : POSITIONT;
    float4 originalPos : POSITION;
};

struct g2pConnector
{
    float4 color : COLOR;
    float4 pos : SV_Position;
};

[maxvertexcount(3)]
void main(
	triangle v2gConnector input[3],
	inout TriangleStream<g2pConnector> output
)
{
    int zeroCount = 0;
	for (uint i = 0; i < 3; i++)
	{
        if (input[i].originalPos.x == 0.f && input[i].originalPos.y == 0.f && input[i].originalPos.z == 0.f)
        {
            ++zeroCount;
        }
    }
    if(zeroCount != 3)
    {
        for (uint i = 0; i < 3; i++)
        {
            g2pConnector g2p;
            g2p.pos = input[i].pos;
            g2p.color = input[i].pos;
            output.Append(g2p);
        }
    }
    output.RestartStrip();
}