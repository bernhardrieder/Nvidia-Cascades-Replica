cbuffer PerApplication : register(b0)
{
    matrix projectionMatrix;
}

cbuffer PerFrame : register(b1)
{
    matrix viewMatrix;
}

cbuffer PerObject : register(b2)
{
    matrix worldMatrix;
}

struct AppData
{
    float3 position : POSITION;
    float3 color : COLOR;
};

struct VertexShaderOutput
{
    float4 color : COLOR;
    float4 pos : SV_POSITION;
};

struct GSOutput
{
    float4 color : COLOR;
    float4 pos : SV_POSITION;
};