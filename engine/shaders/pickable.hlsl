#include "globals.hlsli"
#include "gbuffer.hlsli"

Texture2D<float4> g_colorMap : register(t0);

struct VS_INPUT
{
    float3 position : POSITION;
    float2 tex_coord : TEXCOORD0;
    float3 normal : NORMAL;

    float3 instance_pos : INSTANCE_POS;
    float instance_size : INSTANCE_SIZE;
};

struct PS_INPUT
{
    float4 position : SV_Position;
    float3 normal : NORMAL;
    float2 tex_coord : TEXCOORD0;
};

PS_INPUT vs_main(VS_INPUT input)
{
    PS_INPUT output;
    float3 pos = input.position;
    pos *= input.instance_size;
    pos.y += sin(g_systemTime * 3) / (5 / input.instance_size);
    
    float s, c;
    sincos(g_systemTime, s, c);
    float3x3 rotate_y = float3x3(float3(c, 0, s), float3(0, 1, 0), float3(-s, 0, c));
    pos = mul(pos, rotate_y);
    output.normal = mul(input.normal, rotate_y);
    pos += input.instance_pos;
    output.position = mul(float4(pos, 1), g_viewProj);
    output.tex_coord = input.tex_coord;
    return output;
}

PS_OUTPUT ps_main(PS_INPUT input, bool isFrontFace : SV_IsFrontFace)
{
    float4 color = g_colorMap.Sample(g_samplerState, input.tex_coord);
    if (color.a < 0.5f)
    {
        PS_OUTPUT output;
        discard;
        return output;
    }
    Surface surface;
    surface.albedo = color.xyz;
    surface.emission = color.xyz;
    surface.F0 = 0;
    surface.geometry_normal = input.normal;
    surface.isFrontFace = isFrontFace;
    surface.map_normal = input.normal;
    surface.metalness = 0;
    surface.roughness = 0.2f;
    
    return GBuffer(surface, 0);
}