#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;

layout(location = 0) out vec2 outTexCoord;

layout(set = 0, binding = 0) readonly buffer perframe
{
    mat4 projectViewMatrix;
    vec3 cameraPosition;
    float cameraPositionPadding;
};

void main()
{
    outTexCoord = texCoord;
    gl_Position = projectViewMatrix * vec4(position, 1.0f);
}