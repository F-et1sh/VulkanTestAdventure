#version 460
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTextureCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 textureCoord;

void main() {
    gl_Position = vec4(inPosition, 1.0f);
    fragColor = inColor;
    textureCoord = inTextureCoord;
}
