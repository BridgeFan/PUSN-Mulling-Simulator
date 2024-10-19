#version 420 core
out vec4 FragColor;

in vec4 vertexColor;
in vec3 normal;
in vec3 fragPos;
uniform bool isNormalUsed;
struct Light {
//vec3 position;
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform vec3 viewPos;
uniform Light light;
uniform float shininess;

void main()
{
    if(!isNormalUsed) {
        FragColor = vertexColor;
        return;
    }
    vec3 norm = normalize(normal);
    vec3 color = vertexColor.rgb;

    // ambient
    vec3 ambient = light.ambient * color;

    // diffuse
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * color;

    // specular
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = light.specular * spec * vec3(1.0,1.0,1.0);

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, vertexColor.a);
}