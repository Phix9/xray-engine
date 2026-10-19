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

struct GBufferData
{
    vec3  worldNormal;
    vec3  baseColor;
    float metallic;
    float specular;
    float roughness;
    float AO;
};

const float PI = 3.14159265359;

layout(location = 0) in vec2 inTexCoord;

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

layout(input_attachment_index = 0, set = 1, binding = 0) uniform subpassInput inGBufferA;
layout(input_attachment_index = 1, set = 1, binding = 1) uniform subpassInput inGBufferB;
layout(input_attachment_index = 2, set = 1, binding = 2) uniform subpassInput inGBufferC;
layout(input_attachment_index = 3, set = 1, binding = 3) uniform subpassInput inDepth;

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
    vec4 gBufferA = subpassLoad(inGBufferA).rgba;
    vec4 gBufferB = subpassLoad(inGBufferB).rgba;
    vec4 gBufferC = subpassLoad(inGBufferC).rgba;

    GBufferData gBuffer;
    gBuffer.worldNormal = (gBufferA.rgb) * 2.0 - 1.0;
    gBuffer.metallic = gBufferB.r;
    gBuffer.specular = gBufferB.g;
    gBuffer.roughness = gBufferB.b;
    gBuffer.AO = gBufferB.a;
    gBuffer.baseColor = gBufferC.rgb;

    vec3 N = gBuffer.worldNormal;
    vec3 baseColor = gBuffer.baseColor;
    float metallic = gBuffer.metallic;
    float specular = gBuffer.specular;
    float roughness = gBuffer.roughness;
    float AO = gBuffer.AO;

    float depth = subpassLoad(inDepth).r;
    vec4 ndc = vec4((inTexCoord * 2.0 + vec2(-1.0)), depth, 1.0);
    vec4 worldPositionWithW = inverseProjectViewMatrix * ndc;
    vec3 worldPosition = worldPositionWithW.xyz / worldPositionWithW.w;

    vec3 color = vec3(0.0);
    
    vec3 V = normalize(cameraPosition - worldPosition);
    
    vec3 F0 = vec3(specular);
    F0 = mix(F0, baseColor, metallic);

    vec3 Lo = vec3(0.0);

    for(int i = 0; i < int(pointLightCount) && i < 16; i++) 
    {
        vec3 pointLightPosition = pointLights[i].position;
        float pointLightRadius = pointLights[i].radius;
        vec3 pointLightIntensity = pointLights[i].intensity;

        vec3 L = normalize(pointLightPosition - worldPosition);

        float distance = length(pointLightPosition - worldPosition);
        float attenuation = 1.0 / (distance * distance + 1.0);
        vec3 radiance = pointLightIntensity * attenuation;

        float NdotL = clamp(dot(N, L), 0.0, 1.0);

        Lo += BRDF(L, V, N, F0, baseColor, metallic, roughness) * radiance * NdotL;
    }

    vec3 ambient = ambientLight * baseColor * AO;
    
    color = ambient + Lo;

    outColor = vec4(color, 1.0);
}