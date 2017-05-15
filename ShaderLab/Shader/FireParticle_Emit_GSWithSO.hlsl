#include "FireParticle.hlsli"

[maxvertexcount(2)]
void main(point Particle particle[1], inout PointStream<Particle> outStream)
{
    particle[0].Age += gDeltaTime;
	
    if (particle[0].Type == PT_EMITTER)
    {
		// time to emit a new particle?
        if (particle[0].Age > 0.005f)
        {
            float3 randomVector = RandUnitVec3(gRandomTex, samLinear, gGameTime, gDeltaTime);
            randomVector.x *= 0.2f;
            randomVector.z *= 0.2f;
            float3 randRangeZeroToOne = RandVec3(gRandomTex, samLinear, gGameTime, 0) * 0.5f + 0.5f;

            Particle p;
            p.InitialPosW = float4(gEmitPosW.xyz, 0.f);
            p.InitialVelW = randRangeZeroToOne.y * 4.0f * float4(randomVector.xyz, 0.f);
            p.SizeW = max(float2(1.f, 1.f), float2(randRangeZeroToOne.x, randRangeZeroToOne.x) * 5.f);
            p.Age = 0.f;
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
        if (particle[0].Age <= 1.0f) // == maxLifetime
            outStream.Append(particle[0]);
    }
    outStream.RestartStrip();

}