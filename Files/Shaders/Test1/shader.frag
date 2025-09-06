#version 460
#extension GL_KHR_vulkan_glsl : enable

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 textureCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D u_Texture;

void main() {
    outColor = texture(u_Texture, textureCoord);
}
