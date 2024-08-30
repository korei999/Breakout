#version 300 es
precision mediump float;

in vec4 vPos;

uniform vec3 uColor;

out vec4 outColor;

void
main()
{
    outColor = vec4(uColor, 1.0);
}
