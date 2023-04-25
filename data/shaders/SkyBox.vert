#version 460

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

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

layout (location = 0) out vec3 outUVW;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    outUVW = inPos;
    mat4 projection = ubo.projection;
    projection[1][1] *= -1.0;
    vec4 clipPos = projection * pushConsts.model * vec4(inPos.xyz, 1.0);
    gl_Position = clipPos.xyww;
}