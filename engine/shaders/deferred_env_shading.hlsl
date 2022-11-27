#include "fullscreen.hlsli"
#include "environment.hlsli"
#include "gbuffer_unpack.hlsli"

PS_INPUT vs_main(VS_INPUT input)
{
    return fullscreenVertex(input);
}

float4 ps_main(PS_INPUT input) : SV_Target
{
    Surface surface;
    float3 world_pos;
    
    UnpackGbuffer(input.position.xy, world_pos, surface);
    surface.map_normal = surface.isFrontFace ? surface.map_normal : -surface.map_normal;
    surface.geometry_normal = surface.isFrontFace ? surface.geometry_normal : -surface.geometry_normal;
    
    View view;
    fillViewStructure(view, surface.map_normal, world_pos.xyz);
    
    float4 result = float4(0, 0, 0, 1);
    result.xyz = addEnvironmentReflection(view, surface);
    result.xyz += surface.emission;
    return result;
}