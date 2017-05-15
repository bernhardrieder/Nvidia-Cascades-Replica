#include "FireParticle.hlsli"
    
struct v2gConnector
{
    float3 PosW : POSITION;
    float2 SizeW : SIZE;
    float4 Color : COLOR;
    uint Type : TYPE;
    float DistanceToInitialPosition : DISTANCE;
};

struct g2pConnector
{
    float4 PosH : SV_Position;
    float4 Color : COLOR;
    float2 Tex : TEXCOORD;
    float DistanceToInitialPosition : DISTANCE;
};

[maxvertexcount(4)]
void main(point v2gConnector v2g[1], inout TriangleStream<g2pConnector> outStream)
{
	// do not draw emitter particles.
    if (v2g[0].Type != PT_EMITTER)
    {
		// Compute world matrix so that billboard faces the camera.
        float3 look = normalize(gEyePosW.xyz - v2g[0].PosW);
        float3 right = normalize(cross(float3(0, 1, 0), look));
        float3 up = cross(look, right);
		
		// Compute triangle strip vertices (quad) in world space.
        float halfWidth = 0.5f * v2g[0].SizeW.x;
        float halfHeight = 0.5f * v2g[0].SizeW.y;
	
        float4 v[4];
        v[0] = float4(v2g[0].PosW + halfWidth * right - halfHeight * up, 1.0f);
        v[1] = float4(v2g[0].PosW + halfWidth * right + halfHeight * up, 1.0f);
        v[2] = float4(v2g[0].PosW - halfWidth * right - halfHeight * up, 1.0f);
        v[3] = float4(v2g[0].PosW - halfWidth * right + halfHeight * up, 1.0f);
		
		// Transform quad vertices to world space and output 
		// them as a triangle strip.
        g2pConnector gout;
		[unroll]
        for (int i = 0; i < 4; ++i)
        {
            gout.PosH = mul(gViewProj, v[i]);
            gout.Tex = gQuadTexC[i];
            gout.Color = v2g[0].Color;
            gout.DistanceToInitialPosition = v2g[0].DistanceToInitialPosition;
            outStream.Append(gout);
        }
        outStream.RestartStrip();
    }
}