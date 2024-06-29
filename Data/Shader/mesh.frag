#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(binding = 3) uniform sampler2D colorMapSampler;

layout(location = 0) in vec3 inFragColor;
layout(location = 1) in vec2 inFragTexCoord;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(colorMapSampler, inFragTexCoord); // outColor = mix(outColor, vec4(1.0, 0.6, 0.5, 1.0), 0.4);

}