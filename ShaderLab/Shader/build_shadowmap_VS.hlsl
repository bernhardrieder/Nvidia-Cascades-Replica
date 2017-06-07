struct VertexIn
{
    float4 posL : POSITION; //local position
    float3 normalL : NORMAL0; //local vertex normal
    float3 SurfaceNormalL : NORMAL1; //local surface normal
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
	float2 Tex  : TEXCOORD;
};

cbuffer PerFrame : register(b0)
{
    float4x4 g_view;
    float4x4 g_viewProj;
    float4 g_sunLightDirection;
    float3 g_worldEyePosition;
    float g_appTime;
    float3 g_screenSize;
    float g_deltaTime;
}

cbuffer PerObject : register(b1)
{
    float4x4 g_world;
    float4x4 g_worldInvTranspose;
    float4x4 g_ShadowTransform;
}

VertexOut main(VertexIn vin)
{
	VertexOut vout;
	
	float3 posW = mul(g_world, float4(vin.posL.xyz, 1.f)).xyz;
	vout.PosH = mul(g_viewProj, float4(posW.xyz, 1.f));
	//vout.Tex  = mul(float4(vin.Tex, 0.0f, 1.0f), gTexTransform).xy;

	return vout;
}