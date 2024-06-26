#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(binding = 0) uniform MVP_UBO
{
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 5) in vec3 inColor;

void main() 
{
	// extrude along normal
    vec4 pos = vec4(inPosition.xyz + (inNormal * 0.02), 1.0);
	gl_Position = ubo.proj * ubo.view * ubo.model * pos;
}
