#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(set = 0, binding = 0) uniform MVP_UBO
{
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
} ubo;

layout(binding = 1) uniform UTIL_UBO
{
    float selected;
} utils_ubo;

layout(binding = 2) uniform sampler2D colorMapSampler;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	outFragColor = vec4(vec3(1.0, 1.0, 1.0), 1.0); 
}