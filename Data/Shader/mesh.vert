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

layout(location = 0) out vec3 outFragColor;
layout(location = 1) out vec2 outFragTexCoord;

void main()
{
    // set vertex position on world
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);

    // output variables for the fragment shader
    outFragColor = inColor;
    outFragTexCoord = inTexCoord;
}