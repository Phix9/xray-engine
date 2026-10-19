#version 450

layout(location = 0) out vec2 outTexCoord;

void main()
{
    vec3 fullscreenTrianglePositions[3] = vec3[3](vec3(3.0, 1.0, 0.5), vec3(-1.0, -3.0, 0.5), vec3(-1.0, 1.0, 0.5));

    vec2 fullscreenTriangleUVs[3] = vec2[3](vec2(2.0, 1.0), vec2(0.0, -1.0), vec2(0.0, 1.0));

    gl_Position  = vec4(fullscreenTrianglePositions[gl_VertexIndex], 1.0);
    outTexCoord = fullscreenTriangleUVs[gl_VertexIndex];
}