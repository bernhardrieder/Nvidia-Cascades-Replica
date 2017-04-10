#include "ShaderLab.hlsli"

VertexShaderOutput main( AppData IN )
{
    VertexShaderOutput OUT;

    matrix mvp = mul( projectionMatrix, mul( viewMatrix, worldMatrix ) );
    OUT.pos = mul( mvp, float4( IN.position, 1.0f ) );
    OUT.color = float4( IN.color, 1.0f );

    return OUT;
}