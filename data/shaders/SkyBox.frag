#version 450

layout (location = 0) out vec4 outColor;

layout (location = 0) in vec3 inUVW;

layout (binding = 1) uniform samplerCube samplerEnv;

vec3 uncharted2Tonemap(vec3 color)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    float W = 11.2;
    return ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
}

void main()
{
    vec3 color = texture(samplerEnv, inUVW).rgb;
    outColor = vec4(color, 1.0);
}