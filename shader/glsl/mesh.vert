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

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 outWorldPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outTangent;
layout(location = 3) out vec2 outTexCoord;

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

void main()
{
    outTexCoord = inTexCoord;
    outWorldPosition = inPosition;
    outTangent = inTangent;
    outNormal = inNormal;
    gl_Position = projectViewMatrix * vec4(inPosition, 1.0f);
}