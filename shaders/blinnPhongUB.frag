#version 300 es
precision mediump float;

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

in vec3 vsPos;
in vec2 vsTex;
in vec3 vsNorm;
in mat4 vsModel;

uniform sampler2D tex;
uniform Light light;
uniform mat3 normMat; /* is needed for non uniform scaling */
uniform vec3 viewPos;

out vec4 fragColor;

vec4
blinnPhong()
{
    float specFactor = 1.0;
    vec3 norm = normalize(normMat * vsNorm);
    vec3 fragPos = vec3(vsModel * vec4(vsPos, 1.0));

    vec3 lightDir = normalize(light.pos - fragPos);

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * light.diffuse;

    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
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
    fragColor = blinnPhong() * texture(tex, vsTex).rgba;
}
