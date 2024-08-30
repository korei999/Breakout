#version 300 es
precision mediump float;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTex;
layout (location = 2) in vec3 aNorm;

struct Light
{
    vec3 pos;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

uniform Light light;

uniform mat4 proj;
uniform mat4 view;
uniform mat4 model;
uniform mat3 normMat; /* is needed for non uniform scaling */
uniform vec3 viewPos;

out vec2 vsTex;
out vec4 vsGouraudColor;

vec4
gouraud()
{
    float specFactor = 1.0;
    vec3 norm = normalize(normMat * aNorm);
    vec3 fragPos = vec3(model * vec4(aPos, 1.0));

    vec3 lightDir = normalize(light.pos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * light.diffuse;

    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = specFactor * spec * light.diffuse;

    float distance = length(light.pos - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    vec3 ambient = light.ambient;

    // ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    vec3 result = (ambient + diffuse + specular);

    return vec4(result, 1.0);
}

void
main()
{
    gl_Position = proj * view * model * vec4(aPos, 1.0);
    vsGouraudColor = gouraud();
    vsTex = aTex;
}
