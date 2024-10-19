#version 420 core
out vec4 FragColor;

in vec4 vertexColor;
//in vec3 normal;
in vec3 fragPos;
in vec2 texPos;

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
uniform vec2 divisions;
uniform vec3 size; //size of whole fragment
uniform sampler2D heightMapFrag;

vec3 heightMapVec(vec2 p) {
    float height = texture(heightMapFrag, p).r;
    vec2 worldPos = (p - 0.5) * size.xy;
    return vec3(worldPos, height * size.z);
}

void main()
{
    // calculate new normal
    vec2 tm = floor(texPos*divisions)/divisions;
    vec2 tp = ceil(texPos*divisions)/divisions;
    vec3 normal;
    vec3 P0 = heightMapVec(tm);
    vec3 P2 = heightMapVec(tp);
    if(texPos.x-tm.x>texPos.y-tm.y) {
        vec3 P1 = heightMapVec(vec2(tp.x,tm.y));
        //lower triangle
        normal = normalize(cross(P1-P0,P2-P0));
    }
else {
        vec3 P3 = heightMapVec(vec2(tm.x,tp.y));
        //upper triangle
        normal = normalize(cross(P2-P0,P3-P0));
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
