#version 460
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTextureCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 textureCoord;

layout(set = 0, binding = 0) uniform UniformBuffer {
    mat4 model;
    mat4 view;
    mat4 proj;
}ubo;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0f);
    fragColor = inColor;
    textureCoord = inTextureCoord;
}
