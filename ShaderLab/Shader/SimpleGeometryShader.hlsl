#include "ShaderLab.hlsli"

Texture3D g_densityTex;
SamplerState g_samLinearWrap;

[maxvertexcount(4)]
void main(
	triangle VertexShaderOutput input[3] /*: SV_POSITION*/,
	inout TriangleStream< GSOutput > output
)
{
	for (uint i = 0; i < 3; i++)
	{
		GSOutput element;
        element.pos = input[i].pos;
        //element.color.xyzw = float4(0.f, 1.f, 0.f, 1.f);
        //element.color = input[i].color;
        element.color = g_densityTex.SampleLevel(g_samLinearWrap, element.pos.xyz, 1);
		output.Append(element);
	}
    GSOutput elementNew;
    elementNew.pos = input[2].pos + float4(0.f, 20.f, 0.f, 1.f);
    elementNew.color = float4(1.f, 0.f, 0.f, 1.f);
    
    output.Append(elementNew);
    //output.RestartStrip();
}