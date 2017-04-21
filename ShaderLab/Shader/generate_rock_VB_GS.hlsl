struct v2gConnector
{
    float3 cubeCorner[8] : POSITION; // corner positions
    float4 densityValueAtCorners0123 : TEX1; // the density values at the 8 cell corners
    float4 densityValueAtCorners4567 : TEX2; // the density values at the 8 cell corners
    uint mc_case : CASE; // 0-255
    float4 densityTexSize : SIZE;
};

struct g2vbConnector
{ 
    // Stream out to a VB & save for reuse!
    float4 wsPosition : SV_POSITION;
    float3 wsNormal : NORMAL;
};

cbuffer cbPerApp : register(b0)
{
    int4 cb_caseToNumpolys[256];
    int4 cb_triTable[1280]; // 256*5 = 1280  (256 cases; up to 15 (0/3/6/9/12/15) verts output for each.)
};

float3 vertexInterpolation(float3 v0, float3 v1, float l0, float l1)
{
    float isolevel = 0.f;
    float percent = (isolevel - l0) / (l1 - l0);
    return lerp(v0, v1, percent);
}

float3 calculateNormal(float3 p1, float3 p2, float3 p3)
{
    float3 normal;

    float3 u = p2 - p1;
    float3 v = p3 - p1;

    normal.x = (u.y * v.z) - (u.z * v.y);
    normal.y = (u.z * v.x) - (u.x * v.z);
    normal.z = (u.x * v.y) - (u.y * v.x);

    return normal;
}

[maxvertexcount(15)]
void main(point v2gConnector input[1], inout TriangleStream<g2vbConnector> outStream)
{
    if (input[0].mc_case != 0 && input[0].mc_case != 255)
    {
        float3 edgeVertexList[12];
        float3 cubeCorner[8] = input[0].cubeCorner;
        float4 densityValueAtCorners0123 = input[0].densityValueAtCorners0123;
        float4 densityValueAtCorners4567 = input[0].densityValueAtCorners4567;

        edgeVertexList[0] = vertexInterpolation(cubeCorner[0], cubeCorner[1], densityValueAtCorners0123.x, densityValueAtCorners0123.y);
        edgeVertexList[1] = vertexInterpolation(cubeCorner[1], cubeCorner[2], densityValueAtCorners0123.y, densityValueAtCorners0123.z);
        edgeVertexList[2] = vertexInterpolation(cubeCorner[2], cubeCorner[3], densityValueAtCorners0123.z, densityValueAtCorners0123.w);
        edgeVertexList[3] = vertexInterpolation(cubeCorner[3], cubeCorner[0], densityValueAtCorners0123.w, densityValueAtCorners0123.x);
        edgeVertexList[4] = vertexInterpolation(cubeCorner[4], cubeCorner[5], densityValueAtCorners4567.x, densityValueAtCorners4567.y);
        edgeVertexList[5] = vertexInterpolation(cubeCorner[5], cubeCorner[6], densityValueAtCorners4567.y, densityValueAtCorners4567.z);
        edgeVertexList[6] = vertexInterpolation(cubeCorner[6], cubeCorner[7], densityValueAtCorners4567.z, densityValueAtCorners4567.w);
        edgeVertexList[7] = vertexInterpolation(cubeCorner[7], cubeCorner[4], densityValueAtCorners4567.w, densityValueAtCorners4567.x);
        edgeVertexList[8] = vertexInterpolation(cubeCorner[0], cubeCorner[4], densityValueAtCorners0123.x, densityValueAtCorners4567.x);
        edgeVertexList[9] = vertexInterpolation(cubeCorner[1], cubeCorner[5], densityValueAtCorners0123.y, densityValueAtCorners4567.y);
        edgeVertexList[10] = vertexInterpolation(cubeCorner[2], cubeCorner[6], densityValueAtCorners0123.z, densityValueAtCorners4567.z);
        edgeVertexList[11] = vertexInterpolation(cubeCorner[3], cubeCorner[7], densityValueAtCorners0123.w, densityValueAtCorners4567.w);
        
        g2vbConnector output;
        output.wsNormal = float3(0, 0, 0);

        uint num_polys = cb_caseToNumpolys[input[0].mc_case].x;
        uint table_pos = input[0].mc_case * 5;

        for (uint tri = 0; tri < num_polys; ++tri)
        {
            int4 triData = cb_triTable[table_pos++];
            
            for (int vertexIndex = 0; vertexIndex < 3; ++vertexIndex)
            {
                output.wsPosition = float4(edgeVertexList[triData[vertexIndex]], 1.f).xzyw;
                output.wsPosition.y *= (2.0f * (input[0].densityTexSize.y / input[0].densityTexSize.x)); // WorldSpaceVolumeHeight -> scale to real position with height == 1
                output.wsNormal = calculateNormal(edgeVertexList[triData.x], edgeVertexList[triData.y], edgeVertexList[triData.z]);
                outStream.Append(output);
            }
            
            outStream.RestartStrip();
        }
    }
}

