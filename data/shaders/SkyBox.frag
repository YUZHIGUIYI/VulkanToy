#version 450

layout (location = 0) out vec4 outColor;

layout (location = 0) in vec3 inUVW;

layout (set = 0, binding = 1) uniform samplerCube samplerEnv;

void main()
{
    vec3 envVector = normalize(inUVW);
    outColor = textureLod(samplerEnv, envVector, 0.0);
}