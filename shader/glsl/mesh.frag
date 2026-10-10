#version 450

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) readonly buffer perframe
{
    mat4 projectViewMatrix;
    vec3 cameraPosition;
    float cameraPositionPadding;
};

layout(set = 1, binding = 0) uniform sampler2D texSampler;

void main()
{
    outColor = texture(texSampler, texCoord);
}