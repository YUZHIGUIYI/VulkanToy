#version 460

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBitangnt;

layout (binding = 0) uniform UBO
{
    mat4 projection;
    mat4 view;
    vec3 camPos;
} ubo;

layout (push_constant) uniform PushConsts
{
    mat4 model;
} pushConsts;

layout (location = 0) out vec3 outWorldPos;
layout (location = 1) out vec2 outUV;
layout (location = 2) out mat3 outTangentBasis;

void main()
{
    vec3 localPos = vec3(pushConsts.model * vec4(inPos, 1.0));

    outWorldPos = localPos;
    outUV = inUV;
    outTangentBasis = mat3(pushConsts.model) * mat3(inTangent, inBitangnt, inNormal);

    gl_Position = ubo.projection * ubo.view * vec4(outWorldPos, 1.0);
}