#version 420 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;

out vec4 vertexColor;
out vec3 normal;
out vec3 fragPos;
//common uniforms
uniform mat4 view;
uniform mat4 projection;
//unique uniforms
uniform vec3 color;
uniform mat4 model;
uniform float cameraDistance;

void main()
{
    gl_Position = (projection * view * model * vec4(aPos * cameraDistance, 1.0f));
    vertexColor = vec4(color, 1.0);
    normal = aNormal;
    fragPos = gl_Position.xyz;
}

