#pragma once

#include "adt/Vec.hh"
#include "math.hh"
#include "gl/gl.hh"
#include "adt/String.hh"

struct Shader;

extern Vec<Shader> g_aAllShaders;

struct Shader
{
    GLuint id = 0;

    Shader() = default;
    Shader(String vertShaderPath, String fragShaderPath);
    Shader(String vertShaderPath, String geomShaderPath, String fragShaderPath);
};

void ShaderSetM3(Shader* s, String name, const math::M3& m);
void ShaderSetM4(Shader* s, String name, const math::M4& m);
void ShaderSetV3(Shader* s, String name, const math::V3& v);
void ShaderSetV4(Shader* s, String name, const math::V4& v);
void ShaderSetI(Shader* s, String name, const GLint i);
void ShaderSetF(Shader* s, String name, const f32 f);
void ShaderLoad(Shader* s, String vertexPath, String fragmentPath);
void ShaderLoad(Shader* s, String vertexPath, String geometryPath, String fragmentPath);
void ShaderUse(Shader* s);
void ShaderQueryActiveUniforms(Shader* s);
void ShaderDestroy(Shader* s);
