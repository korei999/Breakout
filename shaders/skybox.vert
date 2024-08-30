#version 300 es
precision mediump float;

layout (location = 0) in vec3 aPos;

layout (std140) uniform ubProjView
{
    mat4 uProj;
    mat4 uView;
};

uniform mat4 uViewNoTranslate;

out vec3 vsTex;

void
main()
{
    vsTex = aPos;
    gl_Position = uProj * uViewNoTranslate * vec4(aPos, 1.0);
}
