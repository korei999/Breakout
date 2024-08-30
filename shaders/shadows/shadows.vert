#version 300 es

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
layout (location = 2) in vec3 aNormal;

layout (std140) uniform ubProjView
{
    mat4 uProj;
    mat4 uView;
};

uniform mat4 uModel;
uniform mat3 uNormalMatrix;
uniform mat4 uLightSpaceMatrix;

out vec3 vFragPos;
out vec3 vNormal;
out vec2 vTexCoords;
out vec4 vFragPosLightSpace;

void
main()
{
	vFragPos = vec3(uModel * vec4(aPos, 1.0));
	vNormal = uNormalMatrix * aNormal;
	vTexCoords = aTexCoords;
	vFragPosLightSpace = uLightSpaceMatrix * vec4(vFragPos, 1.0);

	gl_Position = uProj * uView * uModel * vec4(aPos, 1.0);
}
