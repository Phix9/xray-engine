#version 450

struct DirectionalLight
{
    vec3 direction;
    float direction_padding;
    vec3 color;
    float color_padding;
};

struct PointLight
{
    vec3 position;
    float radius;
    vec3 intensity;
    float intensity_padding;
};

const float PI = 3.14159265359;

layout(location = 0) in vec3 inWorldPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) readonly buffer perframe
{
    mat4 projectViewMatrix;
    mat4 inverseProjectViewMatrix;
    vec3 cameraPosition;
    float cameraPosition_padding;
    vec3 ambientLight;
    float ambientLight_padding;
    uint pointLightCount;
    uint pointLightCount_padding1;
    uint pointLightCount_padding2;
    uint pointLightCount_padding3;
    PointLight pointLights[16];
    DirectionalLight directionLight;
};

layout(set = 1, binding = 0) uniform sampler2D baseColorTexture;
layout(set = 1, binding = 1) uniform sampler2D metallicTexture;
layout(set = 1, binding = 2) uniform sampler2D roughnessTexture;
layout(set = 1, binding = 3) uniform sampler2D normalTexture;
layout(set = 1, binding = 4) uniform sampler2D AOTexture;

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

float DistributionGGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickSmithGGX(float NdotV, float NdotL, float roughness)
{
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;

    float ggx1 = NdotV / (NdotV * (1.0 - k) + k);
    float ggx2 = NdotL / (NdotL * (1.0 - k) + k);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 BRDF(vec3 L, vec3 V, vec3 N, vec3 F0, vec3 baseColor, float metallic, float roughness)
{
    vec3 H = normalize(V + L);
    float NdotV = clamp(dot(N, V), 0.0, 1.0);
    float NdotL = clamp(dot(N, L), 0.0, 1.0);
    float NdotH = clamp(dot(N, H), 0.0, 1.0);
    float HdotV = clamp(dot(H, V), 0.0, 1.0);

    float D = DistributionGGX(NdotH, roughness);
    float G = GeometrySchlickSmithGGX(NdotV, NdotL, roughness);
    vec3 F = FresnelSchlick(HdotV, F0);

    vec3 specular = D * F * G / (4.0 * NdotL * NdotV + 0.001);
    vec3 kd = (vec3(1.0) - F) * (1.0 - metallic);

    vec3 color = vec3(0.0);
    color += kd * baseColor / PI + (1.0 - kd) * specular;

    return color;
}

void main()
{   
    vec3 baseColor = getBaseColor();
    float metallic = texture(metallicTexture, inTexCoord).z;
    float roughness = texture(roughnessTexture, inTexCoord).z;
    float AO = texture(AOTexture, inTexCoord).z;

    float dielectricSpecular = 0.04;

    vec3 color = vec3(0.0);
    
    vec3 N = calculateNormal();
    vec3 V = normalize(cameraPosition - inWorldPosition);
    //vec3 R = reflect(-V, N);
    
    vec3 F0 = vec3(dielectricSpecular);
    F0 = mix(F0, baseColor, metallic);

    vec3 Lo = vec3(0.0);

    for(int i = 0; i < int(pointLightCount) && i < 16; i++) 
    {
        vec3 pointLightPosition = pointLights[i].position;
        float pointLightRadius = pointLights[i].radius;
        vec3 pointLightIntensity = pointLights[i].intensity;

        vec3 L = normalize(pointLightPosition - inWorldPosition);

        float distance = length(pointLightPosition - inWorldPosition);
        float attenuation = 1.0 / (distance * distance + 1.0);
        vec3 radiance = pointLightIntensity * attenuation;

        float NdotL = clamp(dot(N, L), 0.0, 1.0);

        Lo += BRDF(L, V, N, F0, baseColor, metallic, roughness) * radiance * NdotL;
    }

    vec3 ambient = ambientLight * baseColor * AO;
    
    color = ambient + Lo;

    outColor = vec4(color, 1.0);
}