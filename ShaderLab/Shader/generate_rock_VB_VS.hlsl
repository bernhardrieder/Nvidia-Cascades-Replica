
struct a2vConnector
{
    float4 vertexPos : Position; // -1 .. 1 range
    uint nInstanceID: SV_InstanceID; // drawinstanced ID
};

struct v2gConnector
{
    float3 cubeCorner[8] : POSITION; // corner positions
    float4 densityValueAtCorners0123 : TEX1; // the density values at the 8 cell corners
    float4 densityValueAtCorners4567 : TEX2; // the density values at the 8 cell corners
    uint mc_case: CASE; // 0-255
    float4 densityTexSize : SIZE;
};

Texture3D<float> densityTex : register(t0); // our volume of density values. (+=rock, -=air)
SamplerState samplerTrilinearInterp : register(s0); // trilinearinterpolation; clamps on XY, wraps on Z.

cbuffer cbPerApp : register(b0)
{
    //step from one corner to another
    float4 cornerStep[8];
}

v2gConnector main(a2vConnector a2v)
{
    v2gConnector v2g = (v2gConnector)0;

    densityTex.GetDimensions(v2g.densityTexSize.x, v2g.densityTexSize.z, v2g.densityTexSize.y);

    float4 position = float4(a2v.vertexPos.x, (a2v.nInstanceID / v2g.densityTexSize.x) * 2 - 1, a2v.vertexPos.z, 1); // ...Size.x) * 2 - 1 -> range -1 .. 1
    
    // sample the 3D texture to get the density values at the 8 corners
    float cubeConrnerDensity[8];
    for (int i = 0; i < 8; i++)
    {
        v2g.cubeCorner[i].xyz = position.xyz + cornerStep[i].xyz;
        cubeConrnerDensity[i] = densityTex.SampleLevel(samplerTrilinearInterp, (v2g.cubeCorner[i].xyz * 0.5f) + 0.5f, 0); // ...* 0.5f) + 0.5f -> range 0 .. 1
    }
    
    v2g.densityValueAtCorners0123 = float4(cubeConrnerDensity[0], cubeConrnerDensity[1], cubeConrnerDensity[2], cubeConrnerDensity[3]);
    v2g.densityValueAtCorners4567 = float4(cubeConrnerDensity[4], cubeConrnerDensity[5], cubeConrnerDensity[6], cubeConrnerDensity[7]);

    // determine which of the 256 marching cubes cases we have forthis cell:
    // assumes a isoLevel of 0
    uint4 n0123 = (uint4) saturate(v2g.densityValueAtCorners0123 * 99999);
    uint4 n4567 = (uint4) saturate(v2g.densityValueAtCorners4567 * 99999);
    
    v2g.mc_case =   (n0123.x)      | (n0123.y << 1) | (n0123.z << 2) | (n0123.w << 3) |
                    (n4567.x << 4) | (n4567.y << 5) | (n4567.z << 6) | (n4567.w << 7);

    return v2g;
}