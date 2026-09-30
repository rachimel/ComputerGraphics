#version 460 core

uniform vec3 a_Color;

out vec4 FragColor;

void main()
{
    FragColor = vec4(a_Color, 1.0);
}