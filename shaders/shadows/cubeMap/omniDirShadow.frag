#version 320 es
precision highp float;

in VOut {
    vec3 fragPos;
    vec3 norm;
    vec2 tex;
} vIn;

uniform sampler2D uDiffuseTex;
uniform samplerCube uDepthMap;

uniform vec3 uLightPos;
uniform vec3 uLightColor;
uniform vec3 uViewPos;

uniform float uFarPlane;

out vec4 outColor;

vec3 sampleOffsetDirections[20] = vec3[](
    vec3( 1, 1, 1), vec3( 1,-1, 1), vec3(-1,-1, 1), vec3(-1, 1, 1),
    vec3( 1, 1,-1), vec3( 1,-1,-1), vec3(-1,-1,-1), vec3(-1, 1,-1),
    vec3( 1, 1, 0), vec3( 1,-1, 0), vec3(-1,-1, 0), vec3(-1, 1, 0),
    vec3( 1, 0, 1), vec3(-1, 0, 1), vec3( 1, 0,-1), vec3(-1, 0,-1),
    vec3( 0, 1, 1), vec3( 0,-1, 1), vec3( 0,-1,-1), vec3( 0, 1,-1)
);

// vec3 sampleOffsetDirections[9] = vec3[](
//     vec3(-1, 1, 0), vec3(0, 1, 0), vec3(1, 1, 0),
//     vec3(-1, 0, 0), vec3(0, 0, 0), vec3(0, 1, 0),
//     vec3(-1,-1, 0), vec3(0,-1, 0), vec3(1,-1, 0)
// );

float
shadowCalculation(vec3 fragPos)
{
    vec3 fragToLight = fragPos - uLightPos;
    float currentDepth = length(fragToLight);

    float shadow = 0.0;
    float bias = 0.02;
    float offset = 0.1;
    int samples = sampleOffsetDirections.length();
    float viewDist = length(uViewPos - fragPos);
    float diskRadius = (1.0 + (viewDist / uFarPlane)) * 0.10;

    for (int i = 0; i < samples; i++)
    {
        float closestDepth = texture(uDepthMap, fragToLight + sampleOffsetDirections[i] * diskRadius).r;
        closestDepth *= uFarPlane;
        if (currentDepth - bias > closestDepth)
            shadow += 1.0;
    }

    return shadow * (1.0 / float(samples));
}

void
main()
{
    vec4 color = texture(uDiffuseTex, vIn.tex);

    vec3 normal = normalize(vIn.norm);
    vec3 lightColor = uLightColor;
    /* ambient */
    vec3 ambient = 0.55 * color.rgb;
    /* diffuse */
    vec3 lightDir = normalize(uLightPos - vIn.fragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    /* specular */
    vec3 viewDir = normalize(uViewPos - vIn.fragPos);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);
    spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * lightColor;
    /* calculate shadow */
    float shadow = shadowCalculation(vIn.fragPos);
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color.rgb;

    if (color.a < 0.1)
        discard;

    outColor = vec4(lighting, color.a);
}
