#version 450

layout (location = 0) in vec3 fragColor;
layout (location = 1) in vec2 fragTexCoord;
layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D textureSampler;

layout (push_constant) uniform Push {
    mat4 transform;
    vec3 color;
} push;

void main() {
    vec4 sampledColor = texture(textureSampler, fragTexCoord);
    outColor = vec4(fragColor, 1.0) * sampledColor;
}