#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(set = 0, binding = 0) uniform MVP_UBO
{
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
} ubo;

layout(set = 0, binding = 1) uniform UTIL_UBO
{
    float selected;
    float picking;
    vec2 mousePos;
    vec2 windowSize;
} utils_ubo;

layout(binding = 2) uniform sampler2D colorMapSampler;

layout(location = 0) in vec3 inFragColor;
layout(location = 1) in vec2 inFragTexCoord;

layout(location = 0) out vec4 outColor;

void main()
{
    //object is selected
    if(utils_ubo.selected == 1.0)
    {
        outColor = texture(colorMapSampler, inFragTexCoord);
        outColor = mix(outColor, vec4(1.0, 0.6, 0.5, 1.0), 0.4);
    }

    // output
    else
    {
        outColor = texture(colorMapSampler, inFragTexCoord);
    }
}