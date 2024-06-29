#version 450
#extension GL_KHR_vulkan_glsl : enable

layout(set = 0, binding = 0) uniform ubo
{
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    vec3 cameraFront;
} camera;

layout(push_constant) uniform constants
{
    uint id;
    mat4 model;
} pushConstant;

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 out_uvw;

void main() 
{
    // calculate the fixed correct position
    vec3 position = mat3(pushConstant.model * camera.view) * inPosition;
    gl_Position = (camera.proj * vec4(position, 0.0)).xyzz;

    // pass world coordinate to fragment shader
    out_uvw = inPosition;
}