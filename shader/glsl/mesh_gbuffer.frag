#version 450

struct GBufferData
{
    vec3  worldNormal;
    vec3  baseColor;
    float metallic;
    float specular;
    float roughness;
    float AO;
};

layout(location = 0) in vec3 inWorldPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec4 outGBufferA;
layout(location = 1) out vec4 outGBufferB;
layout(location = 2) out vec4 outGBufferC;

layout(set = 1, binding = 0) uniform sampler2D baseColorTexture;
layout(set = 1, binding = 1) uniform sampler2D metallicRoughnessTexture;
layout(set = 1, binding = 2) uniform sampler2D normalTexture;
layout(set = 1, binding = 3) uniform sampler2D occlusionTexture;
layout(set = 1, binding = 4) uniform sampler2D emissiveTexture;

layout(set = 1, binding = 5) uniform MaterialUBO
{
    vec4 baseColorFactor;
    vec4 emissiveFactor;
    float metallicFactor;
    float roughnessFactor;
    float normalScale;
    float occlusionStrength;
} material;

const float PI = 3.14159265359;

vec3 getBaseColor()
{
    vec3 baseColor = texture(baseColorTexture, inTexCoord).rgb * material.baseColorFactor.rgb;
    return baseColor;
}

vec3 calculateNormal()
{
    vec3 tangentNormal = texture(normalTexture, inTexCoord).xyz * 2.0 - 1.0;
    tangentNormal.xy *= material.normalScale;

    vec3 N = normalize(inNormal);
    vec3 T = normalize(inTangent.xyz);
    vec3 B = normalize(cross(N, T));

    mat3 TBN = mat3(T, B, N);
    return normalize(TBN * tangentNormal);
}

void main()
{
    vec4 metallicRoughness = texture(metallicRoughnessTexture, inTexCoord);
    float ao = texture(occlusionTexture, inTexCoord).r;
    ao = mix(1.0, ao, material.occlusionStrength);

    vec3 emissive = texture(emissiveTexture, inTexCoord).rgb * material.emissiveFactor.rgb;

    GBufferData gBuffer;
    gBuffer.worldNormal = calculateNormal();
    gBuffer.baseColor = getBaseColor() + emissive;
    gBuffer.metallic = metallicRoughness.b * material.metallicFactor;
    gBuffer.specular = 0.04;
    gBuffer.roughness = metallicRoughness.g * material.roughnessFactor;
    gBuffer.AO = ao;
    
    outGBufferA.rgb = gBuffer.worldNormal * 0.5 + 0.5;
    outGBufferB.r = gBuffer.metallic;
    outGBufferB.g = gBuffer.specular;
    outGBufferB.b = gBuffer.roughness;
    outGBufferB.a = gBuffer.AO;
    outGBufferC.rgb = gBuffer.baseColor;
}