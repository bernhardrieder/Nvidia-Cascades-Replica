#include "FireParticle.hlsli"

struct v2gConnector
{
    float3 PosW : POSITION;
    float2 SizeW : SIZE;
    float4 Color : COLOR;
    uint Type : TYPE;
};

v2gConnector main(Particle particle)
{
    v2gConnector v2g;
	
    float t = particle.Age;
	
	// constant acceleration equation
    v2g.PosW = 0.5f * t * t * gAccelW + t * particle.InitialVelW.xyz + particle.InitialPosW.xyz;
	
	// fade color with time
    float opacity = 1.0f - smoothstep(0.0f, 1.0f, t / 1.0f);
    //float opacity = 1.f;
    v2g.Color = float4(1.0f, 1.0f, 1.0f, opacity);
	
    v2g.SizeW = particle.SizeW;
    v2g.Type = particle.Type;
	
    return v2g;
}