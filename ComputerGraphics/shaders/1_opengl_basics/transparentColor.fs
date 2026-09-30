#version 460 core

uniform vec4 a_Color;

out vec4 FragColor;

void main()
{
    FragColor = a_Color;
}