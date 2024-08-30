#version 300 es
precision mediump float;

in vec2 vsTex;
in vec4 vsGouraudColor;

uniform sampler2D tex;

out vec4 fragColor;

void
main()
{
    fragColor = vsGouraudColor * texture(tex, vsTex).rgba;
}
