#version 300 es
precision mediump float;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTex;

out vec2 vsTex;

void
main()
{
    vsTex = aTex;
    gl_Position = vec4(aPos.x, aPos.y, 0.5, 1.0);
}
