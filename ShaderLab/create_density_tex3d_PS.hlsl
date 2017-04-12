
struct g2pConnector
{
    float4 pos : TEXCOORD;
    uint index : SV_RenderTargetArrayIndex;
    float4 rsCoord : SV_POSITION;
};

float main(g2pConnector input) : SV_TARGET
{    
    float density = 0.f;
    //float distToCenter = distance(input.pos.xy, float2(0.f, 0.f));
    ///** filled circle */
    //if (distToCenter <= 0.1f)
    //    density = 0.f;

    ///** circle */
    //if (distToCenter <= 0.9f && distToCenter >= 0.85f)
    //    density = 0.f;

    //tweak input
    input.pos.xy *= 2.0f; // xy scaling
    input.pos.z *= 0.5f;

    //pillar (slide 12)
    density += 1 / length(input.pos.xy - float2(-0.2f, 0.5f)) - 1;
    density += 1 / length(input.pos.xy - float2(1.f, 0.9f)) - 1;
    density += 1 / length(input.pos.xy - float2(-0.6f, -.5f)) - 1;
    
    //water flow channel (slide 14)
    density -= 1 / length(input.pos.xy) - 1;

    //keep solid rock "in bounds" (slide 15)
    density = density - pow(length(input.pos.xy), 3);

    //helix (slide 16)
    float2 vec = float2(cos(input.pos.z), sin(input.pos.z));
    density += dot(vec, input.pos.xy);

    //shelves (slide 17)
    density += cos(input.pos.z);

    return density;
}