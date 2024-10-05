#version 300 es
precision mediump float;

in vec2 vsTex;

out vec4 fragColor;

uniform sampler2D uTex0;
uniform vec4 uColor;

void
main()
{
    // vec4 col = uColor * vec4(1.0, 1.0, 1.0, texture(uTex0, vsTex).r);
    // vec4 col = uColor * texture(uTex0, vsTex);
    // fragColor = col;

    // vec4 sampled = vec4(1.0, 1.0, 1.0, texture(uTex0, vsTex).r);
    vec4 sampled = vec4(texture(uTex0, vsTex).rrrr);
    fragColor = vec4(uColor.xyz, 1.0) * sampled;
}
