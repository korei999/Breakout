#pragma once

#include "adt/Pool.hh"
#include "adt/String.hh"
#include "adt/math.hh"
#include "gl/gl.hh" /* IWYU pragma: keep */

using namespace adt;

struct Shader;

constexpr u32 SHADER_MAX_COUNT = 256;

extern Pool<Shader, SHADER_MAX_COUNT> g_aAllShaders;

struct ShaderHash
{
    String sKeyWord {};
    u32 vecIdx {};
};

inline bool
operator==(const ShaderHash& l, const ShaderHash& r)
{
    return l.sKeyWord == r.sKeyWord;
}

template<>
inline u64
hash::func(const ShaderHash& x)
{
    return hash::func(x.sKeyWord);
}

struct Shader
{
    GLuint m_id = 0;

    Shader() = default;
    Shader(String vertShaderPath, String fragShaderPath);
    Shader(String vertShaderPath, String geomShaderPath, String fragShaderPath);

    void load(String vertexPath, String fragmentPath);
    void load(String vertexPath, String geometryPath, String fragmentPath);
    void queryActiveUniforms();
    void destroy();

    void use() const { glUseProgram(m_id); }
    
    void 
    setM3(String name, const math::M3& m)
    {
        GLint ul = glGetUniformLocation(m_id, name.data());
        glUniformMatrix3fv(ul, 1, GL_FALSE, (GLfloat*)m.e);
    }
    
    void 
    setM4(String name, const math::M4& m)
    {
        GLint ul = glGetUniformLocation(m_id, name.data());
        glUniformMatrix4fv(ul, 1, GL_FALSE, (GLfloat*)m.e);
    }
    
    void
    setV3(String name, const math::V3& v)
    {
        GLint ul = glGetUniformLocation(m_id, name.data());
        glUniform3fv(ul, 1, (GLfloat*)v.e);
    }
    
    void
    setV4(String name, const math::V4& v)
    {
        GLint ul = glGetUniformLocation(m_id, name.data());
        glUniform4fv(ul, 1, (GLfloat*)v.e);
    }
    
    void
    setI(String name, const GLint i)
    {
        GLint ul = glGetUniformLocation(m_id, name.data());
        glUniform1i(ul, i);
    }
    
    
    void
    setF(String name, const f32 f)
    {
        GLint ul = glGetUniformLocation(m_id, name.data());
        glUniform1f(ul, f);
    }
};
