#version 460

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in mat3 inTangentBasis;

layout (location = 0) out vec4 outColor;

layout (binding = 0) uniform UBO
{
    mat4 projection;
    mat4 view;
    vec3 camPos;
} ubo;

layout (binding = 1) uniform sampler2D albedoTexture;
layout (binding = 2) uniform sampler2D normalTexture;
layout (binding = 3) uniform sampler2D roughnessTexture;
layout (binding = 4) uniform sampler2D metallicTexture;

layout (binding = 5) uniform samplerCube samplerIrradiance;
layout (binding = 6) uniform samplerCube prefilteredMap;
layout (binding = 7) uniform sampler2D samplerBRDFLUT;

layout (push_constant) uniform PushConsts
{
    layout(offset = 64) int id;
} componentID;

#define PI 3.1415926535897932384626433832795
#define ALBEDO pow(texture(albedoTexture, inUV).rgb, vec3(1.0))

// From http://filmicgames.com/archives/75
vec3 uncharted2Tonemap(vec3 x)
{
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

// Normal Distribution function --------------------------------------
float D_GGX(float dotNH, float roughness)
{
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
    return (alpha2) / (PI * denom * denom);
}

// Geometric Shadowing function --------------------------------------
float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float GL = dotNL / (dotNL * (1.0 - k) + k);
    float GV = dotNV / (dotNV * (1.0 - k) + k);
    return GL * GV;
}

// Fresnel function ----------------------------------------------------
vec3 F_Schlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
vec3 F_SchlickR(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 prefilteredReflection(vec3 R, float roughness)
{
//    const float MAX_REFLECTION_LOD = 9.0;
//    float lod = roughness * MAX_REFLECTION_LOD;
//    float lodf = floor(lod);
//    float lodc = ceil(lod);
//    vec3 a = textureLod(prefilteredMap, R, lodf).rgb;
//    vec3 b = textureLod(prefilteredMap, R, lodc).rgb;

    int prefilteredMapLevles = textureQueryLevels(prefilteredMap);
    return textureLod(prefilteredMap, R, roughness * prefilteredMapLevles).rgb;

    //return mix(a, b, lod - lodf);
}

void main()
{
    // Get current fragment's normal and transform to world space
    vec3 N = normalize(2.0 * texture(normalTexture, inUV).rgb - 1.0);
    N = normalize(inTangentBasis * N);

    // Outgoing light direction (vector from world space fragment position to the camera position)
    vec3 V = normalize(ubo.camPos - inWorldPos);
    vec3 R = reflect(V, N);     // TODO: fix me

    float cosLo = max(dot(N, V), 0.0);

    // Specular reflection vector
    vec3 Lr = 2.0 * cosLo * N - V;

    float roughness = texture(roughnessTexture, inUV).r;
    float metallic = texture(metallicTexture, inUV).r;

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, ALBEDO, metallic);

    vec2 brdf = texture(samplerBRDFLUT, vec2(cosLo, roughness)).rg;
    vec3 reflection = prefilteredReflection(-Lr, roughness).rgb;
    vec3 irradiance = texture(samplerIrradiance, N).rgb;

    // Total diffuse contribution based on irradiance
    vec3 diffuse = irradiance * ALBEDO;

    //vec3 F = F_SchlickR(cosLo, F0, roughness);
    vec3 F = F_Schlick(cosLo, F0);

    // Total specular IBL contribution
    vec3 specular = reflection * (F0 * brdf.x + brdf.y);

    // Get diffuse contribution factor (as with direct lighting)
    vec3 kD = mix(vec3(1.0) - F, vec3(0.0), metallic);

    // Total ambient lighting contribution
    vec3 ambient = kD * diffuse + specular;

    vec3 color = ambient;

    // Reinhard tone mapping
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float mappedLuminance = (luminance * (1.0 + luminance / (1.0))) / (1.0 + luminance);
    color = (mappedLuminance / luminance) * color;
    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));

    outColor = vec4(color, 1.0);
}