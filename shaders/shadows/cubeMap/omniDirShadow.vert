#version 320 es

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTex;
layout (location = 2) in vec3 aNorm;

layout (std140) uniform ubProjView
{
    mat4 uProj;
    mat4 uView;
};

uniform mat4 uModel;
uniform mat3 uNormalMatrix;
uniform bool uReverseNorms;

out vec2 vTex;

out VOut {
    vec3 fragPos;
    vec3 norm;
    vec2 tex;
} vOut;

void
main()
{
    vOut.fragPos = vec3(uModel * vec4(aPos, 1.0));

    if (uReverseNorms)
        vOut.norm = uNormalMatrix * (-1.0 * aNorm);
    else
        vOut.norm = uNormalMatrix * aNorm;

    vOut.tex = aTex;
    
    gl_Position = uProj * uView * uModel * vec4(aPos, 1.0);
}
