#version 460 core

layout (location = 0) in vec3 a_Pos;
layout (location = 1) in vec3 a_Color;
layout (location = 2) in vec3 a_Normal;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat3 normalMatrix;

out vec3 Normal;
out vec3 FragPos;

void main()
{
	vec4 viewPos = view * model * vec4(a_Pos, 1.0);
	gl_Position = projection * viewPos;
	Normal = normalMatrix * a_Normal;
	FragPos = vec3(viewPos);
}
