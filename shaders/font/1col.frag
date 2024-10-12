#version 300 es
precision mediump float;

in vec2 vsTex;

out vec4 fragColor;

uniform sampler2D uTex0;
uniform vec4 uColor;

void
main()
{
    vec3 col = texture(uTex0, vsTex).rrr;

    if (col.r < 0.01)
        discard;

    fragColor = uColor * vec4(col, 1.0f);
}
