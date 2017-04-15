#include "generate_rock_globals.hlsli"

struct v2gConnector
{
    float4 wsCoord : POSITION; //coords are for the LOWER-LEFT corner of the cell.
    float3 uvw_3dtex : TEX; //coords are for the LOWER-LEFT corner of the cell.
    float4 field0123 : TEX1; // the density values
    float4 field4567 : TEX2; // at the corners
    uint mc_case : TEX3; // 0-255
};

struct g2vbConnector
{ 
    // Stream out to a VB & save for reuse!
    // .xyz = wsCoord, .w = occlusion
    float4 wsCoord_Ambo : POSITION;
    float3 wsNormal : NORMAL;
};

cbuffer g_mc_lut1
{
    uint case_to_numpolys[256];
    float4 cornerAmask0123[12];
    float4 cornerAmask4567[12];
    float4 cornerBmask0123[12];
    float4 cornerBmask4567[12];
    float3 vec_start[12];
    float3 vec_dir[12];
};

cbuffer g_mc_lut2
{
    int4 g_triTable[1280]; // 256*5 = 1280  (256 cases; up to 15 (0/3/6/9/12/15) verts output for each.)
};

// our volume of density values.
Texture3D tex;
// trilinearinterp; clamps on XY, wraps on Z.
SamplerState s;

//SamplerState Linear_ClampXY_WrapZ;
//SamplerState Nearest_ClampXY_WrapZ;
//SamplerState LinearRepeat;

g2vbConnector PlaceVertOnEdge(v2gConnector input, int edgeNum)
{
    // Along this cell edge, where does the density value hit zero?
    float str0 = dot(cornerAmask0123[edgeNum], input.field0123) + dot(cornerAmask4567[edgeNum], input.field4567);
    float str1 = dot(cornerBmask0123[edgeNum], input.field0123) + dot(cornerBmask4567[edgeNum], input.field4567);
    float t = saturate(str0 / (str0 - str1)); //0..1

    // use that to get wsCoordand uvwcoords
    float3 pos_within_cell = vec_start[edgeNum] + t * vec_dir[edgeNum]; //[0..1]
    float3 wsCoord = input.wsCoord.xyz + pos_within_cell.xyz * wsVoxelSize;
    float3 uvw = input.uvw_3dtex + (pos_within_cell * inv_voxelDimMinusOne.xyz).xzy;

    g2vbConnector output;
    output.wsCoord_Ambo.xyz = wsCoord;
    //output.wsCoord_Ambo.w = grad_ambo_tex.SampleLevel(s, uvw, 0).w;
    //output.wsNormal = ComputeNormal(tex, s, uvw);

    output.wsCoord_Ambo.w = 0.f; //DUMMY
    output.wsNormal = 0.f; // DUMMY
    return output;
}

[maxvertexcount(15)]
void main(point v2gConnector input[1], inout TriangleStream<g2vbConnector> outStream)
{
    g2vbConnector output;
    uint num_polys = case_to_numpolys[input[0].mc_case];
    uint table_pos = input[0].mc_case * 5;
    for (uint p = 0; p < num_polys; p++)
    {
        int4 polydata = g_triTable[table_pos++];
        output = PlaceVertOnEdge(input[0], polydata.x);
        outStream.Append(output);
        output = PlaceVertOnEdge(input[0], polydata.y);
        outStream.Append(output);
        output = PlaceVertOnEdge(input[0], polydata.z);
        outStream.Append(output);
        outStream.RestartStrip();
    }
}

