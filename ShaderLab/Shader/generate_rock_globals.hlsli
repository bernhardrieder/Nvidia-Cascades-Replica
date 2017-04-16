static float WorldSpaceVolumeHeight = 2.0 * (256 / 96.0);
static float3 voxelDim = float3(96, 256, 96);
static float3 voxelDimMinusOne = float3(95, 256, 95);
static float3 wsVoxelSize = 2.0 / 95.0;
static float4 inv_voxelDim = float4(1.0 / voxelDim, 0); 
static float4 inv_voxelDimMinusOne = float4(1.0 / voxelDimMinusOne, 0);