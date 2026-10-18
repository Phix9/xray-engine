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

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec2 texCoord;

layout(location = 0) out vec3 outWorldPosition;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec3 outTangent;
layout(location = 3) out vec2 outTexCoord;

layout(set = 0, binding = 0) readonly buffer perframe
{
    mat4 projectViewMatrix;
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
    outTexCoord = texCoord;
    outWorldPosition = position;
    outTangent = tangent;
    outNormal = normal;
    gl_Position = projectViewMatrix * vec4(position, 1.0f);
}