#version 460 core

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 objectColor;
// uniform vec3 cameraPos;

uniform float ambient;
uniform int shininess;

in vec3 FragPos;
in vec3 Normal;

layout (location = 0) out vec4 FragColor; 
layout (location = 1) out vec4 BrightColor;

void main()
{
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 normal = normalize(Normal);

    // ambient
    vec3 ambientColor = objectColor * ambient;
    // diffuse
    float diffuse = max(dot(lightDir, normal),0.0);
    vec3 diffuseColor = objectColor * diffuse;

    // specular
    vec3 viewDir = normalize(-FragPos);
    vec3 specularDir = reflect(-lightDir, normal);
    float specular = pow(max(dot(specularDir, viewDir), 0.0),shininess);

    vec3 specularColor = specular * lightColor;

    FragColor = vec4(ambientColor + diffuseColor + specularColor, 1.0);
    BrightColor = vec4(0.0);
}

