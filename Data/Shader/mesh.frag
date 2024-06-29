#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(push_constant) uniform constants
{
    uint id;
	mat4 model;
} pushConstant;

layout(set = 0, binding = 1) buffer sbo // writeonly 
{
    vec2 mousePos;
    uint pickingDepth[256];
} storage;

layout(binding = 3) uniform sampler2D colorMapSampler;

layout(location = 0) in vec3 inFragColor;
layout(location = 1) in vec2 inFragTexCoord;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(colorMapSampler, inFragTexCoord); // outColor = mix(outColor, vec4(1.0, 0.6, 0.5, 1.0), 0.4);

    // write object position into the depth buffer
    if(length(storage.mousePos.xy - gl_FragCoord.xy) < 1)
    {
        storage.pickingDepth[uint(gl_FragCoord.z * 256)] = pushConstant.id;
    }
}