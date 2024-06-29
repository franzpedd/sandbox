#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(push_constant) uniform constants
{
    uint id;
	mat4 model;
} pushConstant;

layout(set = 0, binding = 0) uniform ubo
{
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    vec3 cameraFront;
} camera;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 outFragColor;
layout(location = 1) out vec2 outFragTexCoord;

void main()
{
    // set vertex position on world
    gl_Position = camera.proj * camera.view * pushConstant.model * vec4(inPosition, 1.0);

    // output variables for the fragment shader
    outFragTexCoord = inTexCoord;
}