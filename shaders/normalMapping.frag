#version 320 es
precision highp float;

in VOut {
    vec3 fragPos;
    vec2 tex;
    vec3 tanLightPos;
    vec3 tanViewPos;
    vec3 tanFragPos;
} vOut;

uniform sampler2D uDiffuseTex;
uniform sampler2D uNormalMap;

uniform float uFarPlane;

out vec4 outColor;

void
main()
{
    // obtain normal from normal map in range [0,1]
    vec3 normal = texture(uNormalMap, vOut.tex).rgb;
    // transform normal vector to range [-1,1]
    normal = normalize(normal * 2.0 - 1.0);  // this normal is in tangent space
   
    // get diffuse color
    vec3 color = texture(uDiffuseTex, vOut.tex).rgb;
    // ambient
    vec3 ambient = 0.1 * color;
    // diffuse
    vec3 lightDir = normalize(vOut.tanLightPos - vOut.tanFragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * color;
    // specular
    vec3 viewDir = normalize(vOut.tanViewPos - vOut.tanFragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

    vec3 specular = vec3(0.2) * spec;
    outColor = vec4(ambient + diffuse + specular, 1.0);
}
