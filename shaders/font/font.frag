#version 300 es
precision mediump float;

in vec2 vsTex;

out vec4 fragColor;

uniform sampler2D tex0;

uniform vec4 uColor;

void
main()
{
    vec4 col = uColor * texture(tex0, vsTex);
    fragColor = col;

    if (col.r <= 0.01)
        discard;
}
