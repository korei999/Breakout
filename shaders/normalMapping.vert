#version 320 es

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTex;
layout (location = 2) in vec3 aNorm;
layout (location = 3) in vec3 aTan;

layout (std140) uniform ubProjView
{
    mat4 uProj;
    mat4 uView;
};

uniform mat4 uModel;
uniform mat3 uNormalMatrix;

uniform vec3 uLightPos;
uniform vec3 uViewPos;

out vec2 vTex;

out VOut {
    vec3 fragPos;
    vec2 tex;
    vec3 tanLightPos;
    vec3 tanViewPos;
    vec3 tanFragPos;
} vOut;

void
main()
{
    vOut.fragPos = vec3(uModel * vec4(aPos, 1.0));
    vOut.tex = aTex;

    vec3 t = normalize(uNormalMatrix * aTan);
    vec3 n = normalize(uNormalMatrix * aNorm);
    // t = normalize(t - dot(t, n) * n);
    // vec3 bitan = normalize(cross(n, t));
    vec3 b = normalize(uNormalMatrix * aTan);
    mat3 tbn = transpose(mat3(t, b, n));

    vOut.tanLightPos = tbn * uLightPos;
    vOut.tanViewPos = tbn * uViewPos;
    vOut.tanFragPos = tbn * vOut.fragPos;
    
    gl_Position = uProj * uView * uModel * vec4(aPos, 1.0);
}
