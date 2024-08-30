#version 300 es
precision mediump float;

in vec3 vsTex;

uniform samplerCube uSkyBox;

out vec4 fragColor;

void
main()
{
    fragColor = texture(uSkyBox, vsTex);
}
