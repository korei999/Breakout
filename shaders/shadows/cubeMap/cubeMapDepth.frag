#version 320 es
precision lowp float;

in vec4 gFragPos;

uniform vec3 uLightPos;
uniform float uFarPlane;

void
main()
{
    /* get distance between fragement and light source */
    float lightDist = length(gFragPos.xyz - uLightPos);
    /* map to [0, 1] range by dividing by uFarPlane */
    lightDist = lightDist / uFarPlane;
    /* write this as modified depth */
    gl_FragDepth = lightDist;
}
