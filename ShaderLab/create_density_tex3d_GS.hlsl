
struct v2gConnector
{
    float4 ws : TEXCOORD;
    uint sliceID : BLAH;
};

struct g2pConnector
{
    float4 rsCoord : SV_POSITION;
    float4 pos : TEXCOORD;
    uint index : SV_RenderTargetArrayIndex; //This will write your vertex to a specific slice, which you can read in pixel shader too

};

[maxvertexcount(3)]
void main(
	triangle v2gConnector input[3],
	inout TriangleStream<g2pConnector> output
)
{
	for (uint i = 0; i < 3; i++)
	{
        g2pConnector element;
		element.pos = input[i].ws;
        element.index = input[i].sliceID;
        element.rsCoord = input[i].ws;
		output.Append(element);
	}
    output.RestartStrip();
}