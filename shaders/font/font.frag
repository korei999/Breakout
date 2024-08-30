#version 320 es
precision mediump float;

in vec2 vsTex;

out vec4 fragColor;

uniform sampler2D tex0;

void
main()
{
    vec4 col = texture(tex0, vsTex);
    fragColor = col;

    if (col.r <= 0.01)
        discard;
}
