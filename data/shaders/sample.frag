#version 460

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

layout (binding = 0) uniform UBO
{
    mat4 projection;
    mat4 view;
    vec3 camPos;
} ubo;

//layout (binding = 1) uniform sampler2D samplerBRDFLUT;

void main() {
    outColor = vec4(0.6, 0.6, 0.6, 1.0);
    //outColor = texture(samplerBRDFLUT, inUV) * outColor;
}