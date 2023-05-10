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

layout (binding = 5) uniform samplerCube irradianceTexture;
layout (binding = 6) uniform samplerCube specularTexture;
layout (binding = 7) uniform sampler2D specularBRDF_LUT;

layout (push_constant) uniform PushConsts
{
    layout(offset = 64) int id;
} componentID;

#define PI 3.1415926535897932384626433832795

const vec3 Fdielectric = vec3(0.04);

// GGX/Towbridge-Reitz normal Distribution function ------------------
// Use Disney's reparametrization of alpha = roughness^2 -------------
float ndfGGX(float dotNH, float roughness)
{
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
    return (alpha2) / (PI * denom * denom);
}

// Schlick-GGX approximation of geometric shadowing function  --------
float gaSchlicksmithGGX(float dotNL, float dotNV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;    // Epic suggests using this roughness remapping for analytic lights
    float GL = dotNL / (dotNL * (1.0 - k) + k);
    float GV = dotNV / (dotNV * (1.0 - k) + k);
    return GL * GV;
}

// Fresnel function using Shlick's approximation ---------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (vec3(1.0) - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 fresnelSchlickR(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

void main()
{
    // Sample input textures to get albedo, roughness and metallic
    vec3 albedo = texture(albedoTexture, inUV).rgb;
    float roughness = texture(roughnessTexture, inUV).r;
    float metallic = texture(metallicTexture, inUV).r;

    // Calculate current fragment's normal and transform to world space
    vec3 N = normalize(2.0 * texture(normalTexture, inUV).rgb - 1.0);
    N = normalize(inTangentBasis * N);

    // Outgoing light direction - vector from world space fragment position to the camera position
    vec3 Lo = normalize(ubo.camPos - inWorldPos);

    // cosine between surface normal and outgoing light direction
    float cosLo = max(dot(N, Lo), 0.0);

    // Specular reflection vector
    vec3 Lr = 2.0 * cosLo * N - Lo;

    // Fresnel reflectance at normal incidence - for metals use albedo color, metallic = 1.0
    vec3 F0 = mix(Fdielectric, albedo, metallic);

    // Ambient lighting - IBL
    vec3 ambientLighting = vec3(0.0);
    {
        // Sample diffuse irradiance at normal direction
        vec3 irradiance = texture(irradianceTexture, N).rgb;

        // Calculate Fresnel term for ambient lighting
        // Since we use pre-filtered cubemap and irradiance is coming from many directions
        // Uer cosLo instead of angle with light's half-vector (cosLh)
        vec3 F = fresnelSchlick(cosLo, F0);

        // Get diffuse contribution factor - as with direct lighting
        vec3 kd = mix(vec3(1.0) - F, vec3(0.0), metallic);

        // Irradiance map contains exitant radiance assuming Lambertian BRDF, no need to scale by 1/PI here either
        vec3 diffuseIBL = kd * albedo * irradiance;

        // Sample pre-filtered specular reflection environment at correct mipmap level
        int specularTextureLevels = textureQueryLevels(specularTexture);
        vec3 specularIrradiance = textureLod(specularTexture, Lr, roughness * specularTextureLevels).rgb;

        // Split-sum approximation factors for Cook-Torrance specular BRDF
        vec2 specularBRDF = texture(specularBRDF_LUT, vec2(cosLo, roughness)).rg;

        // Total specular IBL contribution
        vec3 specularIBL = (F0 * specularBRDF.x + specularBRDF.y) * specularIrradiance;

        // Total ambient lighting contribution
        ambientLighting = diffuseIBL + specularIBL;
    }

    // TODO: have to fix, since gamma corrextion were applied to non-color textures
    ambientLighting = pow(ambientLighting, vec3(2.2));

    // TODO: add direct lighting calculation for analytical lights

    outColor = vec4(ambientLighting, 1.0);
}