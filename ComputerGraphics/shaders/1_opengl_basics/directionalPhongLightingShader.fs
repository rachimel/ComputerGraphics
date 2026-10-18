#version 460 core

// needs to be normalized when uploading to the shader!
uniform vec3 lightDir;

uniform vec3 lightColor;
uniform vec3 objectColor;

uniform float ambient;
uniform float specularStrength;
uniform int shininess;

in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

void main()
{
    vec3 normal = normalize(Normal);
    // ambient
    vec3 ambientColor = objectColor * ambient;
    // diffuse
    float diffuse = max(dot(-lightDir, normal), 0.0);
    vec3 diffuseColor = objectColor * lightColor * diffuse;

    vec3 viewDir = normalize(-FragPos);
    vec3 specularDir = reflect(lightDir, normal);
    float specular = pow(max(dot(specularDir, viewDir), 0.0), shininess);

    vec3 specularColor = lightColor * specular * specularStrength;
    FragColor = vec4(ambientColor + diffuseColor + specularColor, 1.0);

}