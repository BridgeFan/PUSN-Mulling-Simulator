#version 420 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;

out vec4 vertexColor;
//out vec3 normal;
out vec3 fragPos;
out vec2 texPos;
//common uniforms
uniform mat4 view;
uniform mat4 projection;
//unique uniforms
uniform vec3 color;
uniform sampler2D heightMap;

void main()
{
	//normal = aNormal;
	float z = aPos.z*texture(heightMap, aTexCoord).r;
	//z = aPos.z/2;
	fragPos = vec3(aPos.xy, z);
	texPos = aTexCoord;
	gl_Position = projection * view * vec4(fragPos, 1.0f);
	vertexColor = vec4(color, 1.0);
}

