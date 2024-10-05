#version 300 es

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTex;

out vec2 vsTex;

uniform mat4 uProj;

void
main()
{
    gl_Position = uProj * vec4(aPos, 1.0);
    vsTex = aTex;
}
