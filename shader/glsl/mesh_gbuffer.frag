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
layout(set = 1, binding = 1) uniform sampler2D metallicTexture;
layout(set = 1, binding = 2) uniform sampler2D roughnessTexture;
layout(set = 1, binding = 3) uniform sampler2D normalTexture;
layout(set = 1, binding = 4) uniform sampler2D AOTexture;

const float PI = 3.14159265359;

vec3 getBaseColor()
{
    vec3 baseColor = texture(baseColorTexture, inTexCoord).xyz;
    return baseColor;
}

vec3 calculateNormal()
{
    vec3 tangentNormal = texture(normalTexture, inTexCoord).xyz * 2.0 - 1.0;

    vec3 N = normalize(inNormal);
    vec3 T = normalize(inTangent.xyz);
    vec3 B = normalize(cross(N, T));

    mat3 TBN = mat3(T, B, N);
    return normalize(TBN * tangentNormal);
}

void main()
{
    GBufferData gBuffer;
    gBuffer.worldNormal = calculateNormal();
    gBuffer.baseColor = getBaseColor();
    gBuffer.metallic = texture(metallicTexture, inTexCoord).z;
    gBuffer.specular = 0.04;
    gBuffer.roughness = texture(roughnessTexture, inTexCoord).z;
    gBuffer.AO = texture(AOTexture, inTexCoord).z;
    
    outGBufferA.rgb = gBuffer.worldNormal * 0.5 + 0.5;
    outGBufferB.r = gBuffer.metallic;
    outGBufferB.g = gBuffer.specular;
    outGBufferB.b = gBuffer.roughness;
    outGBufferB.a = gBuffer.AO;
    outGBufferC.rgb = gBuffer.baseColor;
}