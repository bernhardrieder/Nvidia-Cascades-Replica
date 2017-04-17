#include "generate_rock_globals.hlsli"

// This vertex shader is fed 95*95points, one for each *cell* we'll run M.C. on.
// To generate > 1 slice in a single frame, we call DrawInstanced(N),
// and it repeats it N times, each time setting nInstanceIDto [0 .. N-1].

// per-vertex input attributes: [never change]
struct a2vConnector
{
    float3 uv : Position; // 0..1 range
    uint nInstanceID: SV_InstanceID;
};

struct v2gConnector
{ // per-vertex outputs:
    float4 wsCoord : POSITION; // coordsfor LOWER-LEFT corner of the cell ==> range -1 ... +1
    float3 uvw : TEX;
    float4 f0123 : TEX1; // the density values
    float4 f4567 : TEX2; // at the 8 cell corners
    uint mc_case: TEX3; // 0-255
};

Texture3D<float> tex; // our volume of density values. (+=rock, -=air)
SamplerState s; // trilinearinterpolation; clamps on XY, wraps on Z.

cbuffer SliceInfos : register (b0)
{
    // Updated each frame. To generate 5 slices this frame,
    // app has to put their world-space Y coords in slots [0..4] here.
    float4 slice_world_space_Y_coord[256];
}

// converts a point in world space to 3D texture space (for sampling the 3D texture):
#define WS_to_UVW(ws) (float3(ws.xz*0.5f+0.5f, ws.y*WorldSpaceVolumeHeight).xyz) //changed from xzy to xyz!

v2gConnector main(a2vConnector a2v)
{
    // get world-space coordinates & UVW coords of lower-left corner of this cell
    float3 wsCoord;
    wsCoord.xz = a2v.uv.xy * 2 - 1; // convert input uv coords into -1 to +1 range
    //wsCoord.y = slice_world_space_Y_coord[a2v.nInstanceID].y; //old
    //wsCoord.y = a2v.nInstanceID * (1.f / 256.f) * WorldSpaceVolumeHeight + (a2v.nInstanceID) / 256.f;    
    float width, height, depth;
    tex.GetDimensions(width, height, depth);
    wsCoord.y = (WorldSpaceVolumeHeight * (a2v.nInstanceID / depth) + (1 / depth));
    float3 uvw = WS_to_UVW(wsCoord);
    //float3 uvw = float3(a2v.uv.x, a2v.uv.y, wsCoord.y * WorldSpaceVolumeHeight);
    
    // sample the 3D texture to get the density values at the 8 corners
    //float2 step = float2(wsVoxelSize.x, 0); // so used in cascade secrets shader code
    //float2 step = float2(inv_voxelDimMinusOne.x, 0); // inv_voxelDimMinusOne used in cascade shader code ==> makes more sense?!
    //float3 step = float3(1.0f / 95.0f, 0.f, 1.0f / 256.f);
    //TODO: MAYBE CHANgE STEP!!
    //initial version
    //float4 f0123 = float4(  tex.SampleLevel(s, uvw + step.yyy, 0).x,
    //                        tex.SampleLevel(s, uvw + step.yyx, 0).x,
    //                        tex.SampleLevel(s, uvw + step.xyx, 0).x,
    //                        tex.SampleLevel(s, uvw + step.xyy, 0).x);
    //float4 f4567 = float4(  tex.SampleLevel(s, uvw + step.yxy, 0).x,
    //                        tex.SampleLevel(s, uvw + step.yxx, 0).x,
    //                        tex.SampleLevel(s, uvw + step.xxx, 0).x,
    //                        tex.SampleLevel(s, uvw + step.xxy, 0).x);

    //float3 step = float3(1.0f / 95.0f, 0.f, WorldSpaceVolumeHeight * a2v.nInstanceID / 256.f);

    float3 step = float3(1.0f / 95.0f, 0.f, 0);
    float z = uvw.z /100.f ;
    float4 f0123 = float4(tex.SampleLevel(s, float3(uvw.xy, z) + step.yyy, 0).x,
                            tex.SampleLevel(s, float3(uvw.xy, z) + step.yyz, 0).x,
                            tex.SampleLevel(s, float3(uvw.xy, z) + step.xyz, 0).x,
                            tex.SampleLevel(s, float3(uvw.xy, z) + step.xyy, 0).x);
    float4 f4567 = float4(tex.SampleLevel(s, float3(uvw.xy, z) + step.yxy, 0).x,
                            tex.SampleLevel(s, float3(uvw.xy, z) + step.yxz, 0).x,
                            tex.SampleLevel(s, float3(uvw.xy, z) + step.xxz, 0).x,
                            tex.SampleLevel(s, float3(uvw.xy, z) + step.xxy, 0).x);

    //float3 step = float3(1.0f / 95.0f, 0.f, 1.0f / 256.f);
    ////Version 1 - cascade slides NEW
    //float z = uvw.z;
    //float z = 0.f;
    //float4 f0123 = float4(tex.SampleLevel(s, float3(uvw.xy, z) + step.yxy, 0).x,
    //                        tex.SampleLevel(s, float3(uvw.xy, z) + step.xxy, 0).x,
    //                        tex.SampleLevel(s, float3(uvw.xy, z) + step.xyy, 0).x,
    //                        tex.SampleLevel(s, float3(uvw.xy, z) + step.yyy, 0).x);
    //float4 f4567 = float4(tex.SampleLevel(s, float3(uvw.xy, z) + step.yxz, 0).x,
    //                        tex.SampleLevel(s, float3(uvw.xy, z) + step.xxz, 0).x,
    //                        tex.SampleLevel(s, float3(uvw.xy, z) + step.xyz, 0).x,
    //                        tex.SampleLevel(s, float3(uvw.xy, z) + step.yyz, 0).x);

    //version 2
    //float z = (a2v.nInstanceID / 256.f);
    //float z = 0.f;
    //float4 f0123 = float4(  tex.SampleLevel(s, float3(uvw.xy, z) + step.yxy, 0).x,
    //                        tex.SampleLevel(s, float3(uvw.xy, z) + step.yyy, 0).x,
    //                        tex.SampleLevel(s, float3(uvw.xy, z) + step.xyy, 0).x,
    //                        tex.SampleLevel(s, float3(uvw.xy, z) + step.yxy, 0).x);
    //float4 f4567 = float4(  tex.SampleLevel(s, float3(uvw.xy, z) + step.yxz, 0).x,
    //                        tex.SampleLevel(s, float3(uvw.xy, z) + step.yyz, 0).x,
    //                        tex.SampleLevel(s, float3(uvw.xy, z) + step.xyz, 0).x,
    //                        tex.SampleLevel(s, float3(uvw.xy, z) + step.yxz, 0).x);

    ////version 3
    //float width, height, depth;
    //tex.GetDimensions(width, height, depth);
    ////float3 step = float3(1.0f / 95.0f, 0.f, 1.f / depth);
    ////float z = uvw.z / 100.f;

    ////float4 step = float4(inv_voxelDimMinusOne.xzy, 0);
    //float4 step = float4(1.0f / width, 1.0f / height, a2v.nInstanceID / depth, 0.f);
    //float4 f0123;
    //float4 f4567; 
    //float z = 0 ;
    ////float z = 0.f;

    //f0123.x = tex.SampleLevel(s, float3(uvw.xy, z) + step.www, 0).x;
    //f0123.y = tex.SampleLevel(s, float3(uvw.xy, z) + step.wwz, 0).x;
    //f0123.z = tex.SampleLevel(s, float3(uvw.xy, z) + step.xwz, 0).x;
    //f0123.w = tex.SampleLevel(s, float3(uvw.xy, z) + step.xww, 0).x;
    //f4567.x = tex.SampleLevel(s, float3(uvw.xy, z) + step.wyw, 0).x;
    //f4567.y = tex.SampleLevel(s, float3(uvw.xy, z) + step.wyz, 0).x;
    //f4567.z = tex.SampleLevel(s, float3(uvw.xy, z) + step.xyz, 0).x;
    //f4567.w = tex.SampleLevel(s, float3(uvw.xy, z) + step.xyw, 0).x;

    // determine which of the 256 marching cubes cases we have forthis cell:
    uint4 n0123 = (uint4) saturate(f0123 * 99999);
    uint4 n4567 = (uint4) saturate(f4567 * 99999);

    uint mc_case =  (n0123.x     ) | (n0123.y << 1) | (n0123.z << 2) | (n0123.w << 3) | 
                    (n4567.x << 4) | (n4567.y << 5) | (n4567.z << 6) | (n4567.w << 7);
    
    // fill out return struct using these values, then on to the Geometry Shader.
    v2gConnector v2g;
    // from cascade demo: v2f.wsCoord = float4(wsCoord, instance_wsYpos_bottom_of_slice_above[inst]);
    v2g.wsCoord = float4(wsCoord, wsCoord.y+(1.f/256.f));
    v2g.uvw = uvw;
    v2g.f0123 = f0123;
    v2g.f4567 = f4567;
    v2g.mc_case = mc_case;
    return v2g;
}