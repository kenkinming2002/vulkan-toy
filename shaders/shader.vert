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
    mat4 mvp_matrix    = matrices.mvp;
    mat4 model_matrix  = matrices.model;
    mat3 normal_matrix = mat3(transpose(inverse(model_matrix)));

    gl_Position = mvp_matrix * vec4(inPosition, 1.0);

    fragNormal = normal_matrix * inNormal;
    fragColor  = inColor;
    fragUV     = inUV;
    fragPos    = vec3(model_matrix * vec4(inPosition, 1.0));
}
