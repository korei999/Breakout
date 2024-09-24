#pragma once

#include "adt/Vec.hh"
#include "math.hh"
#include "gl/gl.hh"
#include "adt/String.hh"

struct Shader;

extern Vec<Shader> g_aAllShaders;

struct ShaderHash
{
    String sKey {};
    GLuint vecIdx {};
};

inline bool
operator==(const ShaderHash& l, const ShaderHash& r)
{
    return l.sKey == r.sKey;
}

template<>
inline u64
hash::func(const ShaderHash& x)
{
    return hash::func(x.sKey);
}

struct Shader
{
    GLuint id = 0;

    Shader() = default;
    Shader(String vertShaderPath, String fragShaderPath);
    Shader(String vertShaderPath, String geomShaderPath, String fragShaderPath);
};

void ShaderLoad(Shader* s, String vertexPath, String fragmentPath);
void ShaderLoad(Shader* s, String vertexPath, String geometryPath, String fragmentPath);
void ShaderQueryActiveUniforms(Shader* s);
void ShaderDestroy(Shader* s);

inline void
ShaderUse(Shader* s)
{
    glUseProgram(s->id);
}

inline void 
ShaderSetM3(Shader* s, String name, const math::M3& m)
{
    GLint ul = glGetUniformLocation(s->id, name.pData);
    glUniformMatrix3fv(ul, 1, GL_FALSE, (GLfloat*)m.e);
}

inline void 
ShaderSetM4(Shader* s, String name, const math::M4& m)
{
    GLint ul = glGetUniformLocation(s->id, name.pData);
    glUniformMatrix4fv(ul, 1, GL_FALSE, (GLfloat*)m.e);
}

inline void
ShaderSetV3(Shader* s, String name, const math::V3& v)
{
    GLint ul = glGetUniformLocation(s->id, name.pData);
    glUniform3fv(ul, 1, (GLfloat*)v.e);
}

inline void
ShaderSetV4(Shader* s, String name, const math::V4& v)
{
    GLint ul = glGetUniformLocation(s->id, name.pData);
    glUniform4fv(ul, 1, (GLfloat*)v.e);
}

inline void
ShaderSetI(Shader* s, String name, const GLint i)
{
    GLint ul = glGetUniformLocation(s->id, name.pData);
    glUniform1i(ul, i);
}


inline void
ShaderSetF(Shader* s, String name, const f32 f)
{
    GLint ul = glGetUniformLocation(s->id, name.pData);
    glUniform1f(ul, f);
}
