#version 450

layout(push_constant) uniform UniformBufferObject {
    mat4 mvp;
    mat4 model;
} matrices;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec2 inUV;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec3 fragColor;
layout(location = 2) out vec2 fragUV;
layout(location = 3) out vec3 fragPos;

void main() {
    gl_Position = matrices.mvp * vec4(inPosition, 1.0);

    fragNormal = inNormal;
    fragColor  = inColor;
    fragUV     = inUV;
    fragPos    = vec3(matrices.model * vec4(inPosition, 1.0));
}
