struct PixelShaderInput
{
    float4 color : COLOR;
};

float4 ps( PixelShaderInput IN ) : SV_TARGET
{
    //IN.color.xyz = 0.f;
    return IN.color;
}