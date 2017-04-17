struct a2vConnector
{
    float3 ws : Position;
    uint sliceID : SV_InstanceID;
};

struct v2gConnector
{
    float4 ws : TEXCOORD;
    uint sliceID : BLAH;
};

v2gConnector main(a2vConnector input, uint vertexID : SV_VertexID)
{
    v2gConnector output;
    output.ws = float4(input.ws, 1.f);
    output.sliceID = input.sliceID;
	return output;
}