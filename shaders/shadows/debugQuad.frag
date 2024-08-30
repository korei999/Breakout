#version 300 es
precision lowp float;

in vec2 vTexCoords;

uniform sampler2D uDepthMap;
uniform float uNearPlane;
uniform float uFarPlane;

out vec4 outColor;

/* required when using a perspective projection matrix */
// float
// linearizeDepth(float depth)
// {
    // float z = depth * 2.0 - 1.0; // Back to NDC 
    // return (2.0 * uNearPlane * uFarPlane) / (uFarPlane + uNearPlane - z * (uFarPlane - uNearPlane));	
// }

void
main()
{             
    float depthValue = texture(uDepthMap, vTexCoords).r;
    // outColor = vec4(vec3(linearizeDepth(depthValue) / uFarPlane), 1.0); /* perspective */
    outColor = vec4(vec3(depthValue), 1.0); /* orthographic */
}
