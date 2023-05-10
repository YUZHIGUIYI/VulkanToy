#version 450

layout (location = 0) out vec4 outColor;

layout (location = 0) in vec3 inUVW;

layout (set = 0, binding = 1) uniform samplerCube envTexture;

void main()
{
    vec3 envVector = normalize(inUVW);
    outColor = vec4(pow(textureLod(envTexture, envVector, 0.0).rgb, vec3(2.2)), 1.0);
}