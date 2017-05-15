struct Particle
{
    float4 InitialPosW : SV_POSITION;
    float4 InitialVelW : VELOCITY;
    float2 SizeW : SIZE;
    float Age : AGE;
    uint Type : TYPE;
};
  
#define PT_EMITTER 0
#define PT_FLARE 1

cbuffer cbPerFrame : register(b0)
{
    float4x4 gViewProj;
    float4 gEyePosW;
	// for when the emit position/direction is varying
    float4 gEmitPosW;
    float4 gEmitDirW;
    float gGameTime;
    float gDeltaTime;
}

cbuffer cbFixed : register(b1)
{
	// Net constant acceleration used to accerlate the particles.
    float3 gAccelW = { 0.0f, 7.8f, 0.0f };
	
	// Texture coordinates used to stretch texture over quad 
	// when we expand point particle into a quad.
    float2 gQuadTexC[4] =
    {
        float2(0.0f, 1.0f),
		float2(1.0f, 1.0f),
		float2(0.0f, 0.0f),
		float2(1.0f, 0.0f)
    };
};


// Random texture used to generate random numbers in shaders.
Texture1D gRandomTex : register(t0);
// Array of textures for texturing the particles.
//Texture2DArray gTexArray : register(t1);
Texture2D gTex : register(t1);

SamplerState samLinear : register(s0)
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};

float3 RandUnitVec3(Texture1D randomTex, SamplerState linearSampler, float gameTime, float offset)
{
	// Use game time plus offset to sample random texture.
    float u = (gameTime + offset);
	
	// coordinates in [-1,1]
    float3 v = randomTex.SampleLevel(linearSampler, u, 0).xyz;
	
	// project onto unit sphere
    return normalize(v);
}
 