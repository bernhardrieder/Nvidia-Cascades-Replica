struct v2pConnector
{
    float4 posH : SV_Position; //homogeneous clipspace
    float3 posW : POSITION; //worldspace coord
    float3 VertexNormalW : NORMAL0; //worldspace vertex normal
    float3 SurfaceNormalW : NORMAL1; //worldspace surface normal
};

cbuffer PerApplication : register(b0)
{
    float4x4 g_projection;
    float g_discplacementScale;
    float g_initialStepIterations;
    float g_refinementStepIterations;
    float g_parallaxDepth;
}

cbuffer PerFrame : register(b1)
{
    float4x4 g_view;
    float4x4 g_viewProj;
    float4 g_sunLightDirection;
    float3 g_worldEyePosition;
    float g_appTime;
    float3 g_screenSize;
    float g_deltaTime;
}

Texture2D lichen1 : register(t0);
Texture2D lichen2 : register(t1);
Texture2D lichen3 : register(t2);
Texture2D lichen1_disp : register(t3);
Texture2D lichen2_disp : register(t4);
Texture2D lichen3_disp : register(t5);
Texture2D lichen1_bmp : register(t6);
Texture2D lichen2_bmp : register(t7);
Texture2D lichen3_bmp : register(t8);

SamplerState LinearRepeatAnsio : register(s0);
SamplerState LinearRepeat : register(s1);

#define TEXTURE_SCALE 0.2
#define FLAT_SHADING 0

float3 AddParallax(float2 coord, Texture2D bumpMap, float3 toEyeVec, float dispStrength)
{
    // PARALLAX
    float h = 1;
    float2 uv = coord;
    float prev_hits;
    float ddh = 1.0 / (float) g_initialStepIterations;
    float dd_texels_max = -toEyeVec.xy * dispStrength;
    float2 dduv_max = 1.0 / 1024.0 * dd_texels_max;
    float2 dduv = dduv_max / (float) g_initialStepIterations;

    // first do some iterations to find the intersection.
    // this will just determine the displacement silhouette,
    //   not the shading - we'll refine/interp this for smooth shading
    //   between two of these iterations.
    prev_hits = 0;
    float hit_h = 0;
    for (int it = 0; it < g_initialStepIterations; it++)
    {
        h -= ddh;
        uv += dduv;
        float h_tex = bumpMap.SampleLevel(LinearRepeat, uv, 0).x; // .x is like orig height, but gauss-blurred 1 pixel.  (more is bad; makes the iterations really obvious)
        float is_first_hit = saturate((h_tex - h - prev_hits) * 4999999); // INTERESTING: TRY *4 INSTEAD
        hit_h += is_first_hit * h;
        prev_hits += is_first_hit;
    }

    // REFINEMENT ITERATIONS
    // NOTE: something in this chunk is causing the grey sparklies...
    h = hit_h + ddh - 0.0085; // back up to before the hit.
    // **extra offset helps alleviate grey sparklie artifacts!
    uv = coord + dduv_max * (1 - h);
    float x = 1.0 / (float) g_refinementStepIterations;
    ddh *= x;
    dduv *= x;
  
    prev_hits = 0;
    hit_h = 0;
    for (int it2 = 0; it2 < g_refinementStepIterations; it2++)
    {
        h -= ddh;
        uv += dduv;
        float h_tex = bumpMap.SampleLevel(LinearRepeat, uv, 0).x; // .x is like orig height, but gauss-blurred 1 pixel.  (more is bad; makes the iterations really obvious)
        float is_first_hit = saturate((h_tex - h - prev_hits) * 4999999); // INTERESTING: TRY *4 INSTEAD
        hit_h += is_first_hit * h;
        prev_hits += is_first_hit;
    }

    // assuming the two points in the second iteration only covered
    // one texel, we can now interpolate between the heights at those
    // two points and get the EXACT intersection point.  
    float h1 = (hit_h - ddh);
    float h2 = (hit_h);
    float v1 = bumpMap.SampleLevel(LinearRepeat, coord + dduv_max * (1 - h1), 0).x; // .x is like orig height, but gauss-blurred 1 pixel.  (more is bad; makes the iterations really obvious)
    float v2 = bumpMap.SampleLevel(LinearRepeat, coord + dduv_max * (1 - h2), 0).x; // .x is like orig height, but gauss-blurred 1 pixel.  (more is bad; makes the iterations really obvious)
    // Q: WHY THE -1???
    float t_interp = saturate((v1 - h1) / (h2 + v1 - h1 - v2) - 1);
    hit_h = (h1 + t_interp * (h2 - h1));

    float2 new_coord = coord + dduv_max * (1 - hit_h);
    return float3(new_coord, hit_h);
}

float4 main(v2pConnector v2p) : SV_Target
{
    //----------------------- DETERMINE NORMAL FOR SHADING ---------------------------
    float3 pixelNormalW; 
#if FLAT_SHADING
    pixelNormalW = v2p.SurfaceNormalW;
#else //smooth shading
    pixelNormalW = v2p.VertexNormalW;
#endif
    
    float3 toEyeVec = normalize(g_worldEyePosition - v2p.posW);
    float toEyeDistance = distance(g_worldEyePosition, v2p.posW);

    //----------------------- BLEND WEIGHTS FOR TRI-PLANAR PROJECTION ---------------------------
    float3 blend_weights = abs(normalize(pixelNormalW)) - 0.2f;
    blend_weights *= 7;
    blend_weights = pow(blend_weights, 3);
    blend_weights = max(0, blend_weights);
    blend_weights /= dot(blend_weights, float3(1.f, 1.f, 1.f));
    
    //----------------------- TRI-PLANAR PROJECTION COORDINATES ---------------------------
    float2 coord1 = v2p.posW.yz * TEXTURE_SCALE;
    float2 coord2 = v2p.posW.xz * TEXTURE_SCALE;
    float2 coord3 = v2p.posW.xy * TEXTURE_SCALE;
    
    //----------------------- PARALLAX MAPPING ---------------------------
    const float parallaxDistanceHighLOD = 2.5f; // dist @ which it's full-on 
    const float parallaxDistanceLowLOD = 5.0f ; // dist @ which it's first noticeable
    float parallaxEffectLOD = saturate((parallaxDistanceLowLOD - toEyeDistance) / (parallaxDistanceLowLOD - parallaxDistanceHighLOD));
    //parallaxEffectDetail = 1; //<----- define if you want parallax without LOD!

    if (parallaxEffectLOD > 0)
    {
        float3 toEyeVecWithLOD = toEyeVec * parallaxEffectLOD;
        const float3 parallax_str = abs(pixelNormalW) * g_discplacementScale.xxx * g_parallaxDepth.xxx;

        if (parallax_str.x * blend_weights.x > 0)
            coord1 = AddParallax(coord1, lichen1_disp, toEyeVecWithLOD.yzx, parallax_str.x).xy;
        if (parallax_str.y * blend_weights.y > 0)
            coord2 = AddParallax(coord2, lichen2_disp, toEyeVecWithLOD.xzy, parallax_str.y).xy;
        if (parallax_str.z * blend_weights.z > 0)
            coord3 = AddParallax(coord3, lichen3_disp, toEyeVecWithLOD.xyz, parallax_str.z).xy;
    }
    
    //----------------------- SAMPLE AND BLEND COLOR FROM TEXTURES ---------------------------
    float4 mossCol1 = lichen1.Sample(LinearRepeatAnsio, coord1);
    float4 mossCol2 = lichen2.Sample(LinearRepeatAnsio, coord2);
    float4 mossCol3 = lichen3.Sample(LinearRepeatAnsio, coord3);

    float4 blendedColor = mossCol1.xyzw * blend_weights.xxxx +
                          mossCol2.xyzw * blend_weights.yyyy +
                          mossCol3.xyzw * blend_weights.zzzz;
    
    //----------------------- SAMPLE AND BLEND NORMAL FROM BUMP MAP ---------------------------
    float3 mossBmp[3];
    mossBmp[0].yz = lichen1_bmp.Sample(LinearRepeatAnsio, coord1 * float2(1, 1)).xy - 0.5f;
    mossBmp[0].x = 0;
    mossBmp[1].xz = lichen2_bmp.Sample(LinearRepeatAnsio, coord2 * float2(1, 1)).xy - 0.5f;
    mossBmp[1].y = 0;
    mossBmp[2].xy = lichen3_bmp.Sample(LinearRepeatAnsio, coord3 * float2(1, 1)).xy - 0.5f;
    mossBmp[2].z = 0;

    mossBmp[0].yz *= -1;
    mossBmp[1].xz *= -1;
    mossBmp[2].xy *= -1;

    float3 mossBumpMapNormal = mossBmp[0] * blend_weights.xxx +
                               mossBmp[1] * blend_weights.yyy +
                               mossBmp[2] * blend_weights.zzz;

    //----------------------- SIMPLE LIGHTNING FUNCTION ---------------------------
    float4 lightIntensity = dot(g_sunLightDirection.xyz, normalize(pixelNormalW + mossBumpMapNormal));
    float4 final_color = saturate(lightIntensity * blendedColor);
    float4 ambient = (0.1f * blendedColor);
    final_color = normalize((float4(final_color.xyz, 1) + ambient));
    return final_color;
}