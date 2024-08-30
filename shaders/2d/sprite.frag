#version 300 es
precision highp float;

in vec2 vsTex;

uniform sampler2D tex0;
uniform vec3 uColor;

out vec4 fragColor;

void
main()
{
    vec4 col = texture(tex0, vsTex);
    fragColor = vec4(uColor, 1.0) * col;

    if (col.a < 0.1)
        discard;
}
