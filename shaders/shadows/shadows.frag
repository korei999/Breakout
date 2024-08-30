#version 300 es
precision highp float;

in vec3 vFragPos;
in vec3 vNormal;
in vec2 vTexCoords;
in vec4 vFragPosLightSpace;

uniform sampler2D uDiffuseTexture;
uniform sampler2D uShadowMap;

uniform vec3 uLightPos;
uniform vec3 uViewPos;

out vec4 outColor;

float
shadowCalculation(vec3 lightDir)
{
    /* perform perspective divide */
    vec3 projCoords = vFragPosLightSpace.xyz / vFragPosLightSpace.w;
    /* transform to [0, 1] range */
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0)
        return 0.0;

    /* get closest depth value from light's perspective (using
       [0, 1] range fragPosLight as coords */
    float closestDepth = texture(uShadowMap, projCoords.xy).r;
    /* get depth of current fragment from light's perspective */
    float currentDepth = projCoords.z;
    /* check whether current frag pos is in shadow */
    // float bias = max(0.01 * (1.0 - dot(vNormal, lightDir)), 0.01);
    float bias = 0.0;
    float shadow = 0.0;

    /* PCF */
    vec2 texelSize = 1.0 / vec2(textureSize(uShadowMap, 0));
    for (int x = -1; x <= 1; x++)
        for (int y = -1; y <= 1; y++)
        {
            float pcfDepth = texture(uShadowMap,
                                     projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }

    return shadow * 0.11; /* shadow / 9 */
}

void
main()
{
    vec3 color = texture(uDiffuseTexture, vTexCoords).rgb;
    vec3 normal = normalize(vNormal);
    vec3 lightColor = vec3(1.0);
    /* ambient */
    vec3 ambient = 0.15 * color;
    /* diffuse */
    vec3 lightDir = normalize(uLightPos - vFragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    /* specular */
    vec3 viewDir = normalize(uViewPos - vFragPos);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);
    spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * lightColor;
    /* calculate shadow */
    float shadow = shadowCalculation(lightDir);
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;

    outColor = vec4(lighting, 1.0);
}
