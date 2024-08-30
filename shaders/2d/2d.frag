#version 300 es
precision highp float;

in vec2 vsTex;

uniform sampler2D tex0;
// uniform float nearPlane;
// uniform float farPlane;

out vec4 fragColor;

// float
// linearizeDepth(float depth)
// {
    // float z = depth * 2.0 - 1.0; // Back to NDC 
    // return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));	
// }

void
main()
{
    vec4 col = texture(tex0, vsTex);
    fragColor = col;

    if (col.a < 0.1)
        discard;

    // float depthVal = texture(tex0, vsTex).r;
    // fragColor = vec4(vec3(depthVal), 1.0);
}
