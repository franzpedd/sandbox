#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(push_constant) uniform constants
{
    uint id;
	mat4 model;
} pushConstant;

layout(set = 0, binding = 1) uniform ubo_window
{
    uint selectedID;
    vec2 mousePos;
} window;

#define Z_DEPTH 64 
layout(set = 0, binding = 2) buffer sbo_picking
{
    uint depth[Z_DEPTH];
} picking;

layout(binding = 3) uniform sampler2D colorMapSampler;

layout(location = 0) in vec3 inFragColor;
layout(location = 1) in vec2 inFragTexCoord;

layout(location = 0) out vec4 outColor;

void main()
{
    // write object position into the picking depth buffer
    if(window.mousePos.xy == (gl_FragCoord.xy - 0.5)) // length(window.mousePos.xy - gl_FragCoord.xy) < 1
    {
        uint index = uint(gl_FragCoord.z * Z_DEPTH);
        picking.depth[index] = pushConstant.id;
    }

    outColor = texture(colorMapSampler, inFragTexCoord);

    // selected object
    if(window.selectedID == pushConstant.id)
    {
        outColor = mix(outColor, vec4(1.0, 0.6, 0.5, 1.0), 0.4);
    }
}