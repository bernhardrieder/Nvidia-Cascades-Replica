static float WorldSpaceVolumeHeight = 2.0f * (256.f / 96.0f); // bounding box height in Y
static float3 voxelDim = float3(96, 256, 96);
static float3 voxelDimMinusOne = float3(95, 256, 95);
static float3 wsVoxelSize = 2.0f / 95.0f;
static float4 inv_voxelDim = float4(1.0f / voxelDim, 0); 
static float4 inv_voxelDimMinusOne = float4(1.0f / voxelDimMinusOne, 0);