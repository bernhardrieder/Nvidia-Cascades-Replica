#include "generate_rock_globals.hlsli"

struct a2vConnector
{
    float2 uv : POSITION; // 0..1 range
    uint nInstanceID: SV_InstanceID;
};

struct v2gConnector
{ // per-vertex outputs:
    float3 wsCoord : POSITION; // coordsfor LOWER-LEFT corner of the cell
    float3 uvw : TEX;
    float4 f0123 : TEX1; // the density values
    float4 f4567 : TEX2; // at the 8 cell corners
    uint mc_case: TEX3; // 0-255
};

Texture3D tex; // our volume of density values. (+=rock, -=air)
SamplerState s; // trilinearinterpolation; clamps on XY, wraps on Z.

cbuffer SliceInfos
{
    // Updated each frame. To generate 5 slices this frame,
    // app has to put their world-space Y coords in slots [0..4] here.
    float slice_world_space_Y_coord[256];
}

// converts a point in world space to 3D texture space (for sampling the 3D texture):
#define WS_to_UVW(ws) (float3(ws.xz*0.5+0.5, ws.y*WorldSpaceVolumeHeight).xzy)

v2gConnector main(a2vConnector a2v)
{
    // get world-space coordinates & UVW coords of lower-left corner of this cell
    float3 wsCoord;
    wsCoord.xz = a2v.uv.xy * 2 - 1;
    wsCoord.y = slice_world_space_Y_coord[a2v.nInstanceID];
    float3 uvw = WS_to_UVW(wsCoord);

    // sample the 3D texture to get the density values at the 8 corners
    float2 step = float2(wsVoxelSize.x, 0);
    float4 f0123 = float4(  tex.SampleLevel(s, uvw + step.yyy, 0).x,
                            tex.SampleLevel(s, uvw + step.yyx, 0).x,
                            tex.SampleLevel(s, uvw + step.xyx, 0).x,
                            tex.SampleLevel(s, uvw + step.xyy, 0).x);
    float4 f4567 = float4(  tex.SampleLevel(s, uvw + step.yxy, 0).x,
                            tex.SampleLevel(s, uvw + step.yxx, 0).x,
                            tex.SampleLevel(s, uvw + step.xxx, 0).x,
                            tex.SampleLevel(s, uvw + step.xxy, 0).x);

    // determine which of the 256 marching cubes cases we have forthis cell:
    uint4 n0123 = (uint4) saturate(f0123 * 99999);
    uint4 n4567 = (uint4) saturate(f4567 * 99999);

    uint mc_case =  (n0123.x     ) | (n0123.y << 1) | (n0123.z << 2) | (n0123.w << 3) | 
                    (n4567.x << 4) | (n4567.y << 5) | (n4567.z << 6) | (n4567.w << 7);
    
    // fill out return struct using these values, then on to the Geometry Shader.
    v2gConnector v2g;
    v2g.f0123 = f0123;
    v2g.f4567 = f4567;
    v2g.mc_case = mc_case;
    v2g.uvw = uvw;
    v2g.wsCoord = wsCoord;
    return v2g;

}