static const float SMAP_SIZE = 2048.0f;
static const float SMAP_DX = 1.0f / SMAP_SIZE;

float CalcShadowFactorHard(SamplerState samShadow, Texture2D shadowMap, float4 shadowPosH)
{
    // Complete projection by doing division by w.
    shadowPosH.xyz /= shadowPosH.w;
	
	// Depth in NDC space.
    float depth = shadowPosH.z;

    float sampledValue = shadowMap.Sample(samShadow, shadowPosH.xy).r;

    return depth < sampledValue ? 1 : 0;
}

float CalcShadowFactorPCF(SamplerComparisonState samShadow,
                       Texture2D shadowMap,
					   float4 shadowPosH)
{
	// Complete projection by doing division by w.
    shadowPosH.xyz /= shadowPosH.w;
	
	// Depth in NDC space.
    float depth = shadowPosH.z;

	// Texel size.
    const float dx = SMAP_DX;

    float percentLit = 0.0f;
    const float2 offsets[9] =
    {
        float2(-dx, -dx), float2(0.0f, -dx), float2(dx, -dx),
		float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
		float2(-dx, +dx), float2(0.0f, +dx), float2(dx, +dx)
    };

	[unroll]
    for (int i = 0; i < 9; ++i)
    {
        percentLit += shadowMap.SampleCmpLevelZero(samShadow,
			shadowPosH.xy + offsets[i], depth).r;
    }

    return percentLit /= 9.0f;
}

float CalcShadowFactorESM(SamplerState samShadow, Texture2D shadowMap, float4 shadowPosH)
{
    //https://prezi.com/she-l3p-0xmv/exponential-shadow-mapping/
    //https://pixelstoomany.wordpress.com/2008/06/12/a-conceptually-simpler-way-to-derive-exponential-shadow-maps-sample-code/

    // Complete projection by doing division by w.
    shadowPosH.xyz /= shadowPosH.w;
	
	// Depth in NDC space.
    float receiver = shadowPosH.z;

    //controls the sharpness of our step function approximation and can be used to fake soft shadows
    float k = 75.0f;

    const float dx = SMAP_DX;
    const float2 offsets[9] =
    {
        float2(-dx, -dx), float2(0.0f, -dx), float2(dx, -dx),
		float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
		float2(-dx, +dx), float2(0.0f, +dx), float2(dx, +dx)
    };
    
    float litFactor = 0.f;
    [unroll]
    for (int i = 0; i < 9; ++i)
    {
        float occluder = shadowMap.Sample(samShadow, shadowPosH.xy + offsets[i]).r;
        litFactor += exp(k * occluder);
    }
    litFactor /= 9;
    litFactor /= exp(k * receiver);

    return saturate(litFactor);
}