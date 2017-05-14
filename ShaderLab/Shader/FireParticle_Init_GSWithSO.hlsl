#include "FireParticle.hlsli"

// The stream-out GS is just responsible for emitting 
// new particles and destroying old particles.  The logic
// programed here will generally vary from particle system
// to particle system, as the destroy/spawn rules will be 
// different.
[maxvertexcount(2)]
void main(point Particle particle[1], inout TriangleStream<Particle> outStream)
{
    particle[0].Age += gTimeStep;
	
    if (particle[0].Type == PT_EMITTER)
    {
		// time to emit a new particle?
        if (particle[0].Age > 0.005f)
        {
            
            float3 vRandom = RandUnitVec3(gRandomTex, samLinear, gGameTime, 0);
            vRandom.x *= 0.5f;
            vRandom.z *= 0.5f;
			
            Particle p;
            p.InitialPosW = gEmitPosW.xyz;
            p.InitialVelW = 4.0f * vRandom;
            p.SizeW = float2(3.0f, 3.0f);
            p.Age = 0.0f;
            p.Type = PT_FLARE;
			
            outStream.Append(p);
			
			// reset the time to emit
            particle[0].Age = 0.0f;
        }
		
		// always keep emitters
        outStream.Append(particle[0]);
    }
    else
    {
		// Specify conditions to keep particle; this may vary from system to system.
        if (particle[0].Age <= 1.0f)
            outStream.Append(particle[0]);
    }
}