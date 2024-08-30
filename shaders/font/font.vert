#version 320 es
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTex;

uniform mat4 uProj;

out vec2 vsTex;

void
main()
{
    gl_Position = uProj * vec4(aPos, 1.0, 1.0);
    vsTex = aTex;
}
