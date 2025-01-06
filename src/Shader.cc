#include "Shader.hh"

#include "adt/Arena.hh"
#include "adt/file.hh"
#include "adt/logs.hh"

Pool<Shader, SHADER_MAX_COUNT> g_aAllShaders(INIT);

static GLuint ShaderLoadOne(GLenum type, String path);
static mtx_t s_mtxAllShaders;

static void loadVertFrag(Shader* s, String vertexPath, String fragmentPath);
static void loadVertGeomFrag(Shader* s, String vertexPath, String geometryPath, String fragmentPath);

static void
addToMap()
{
    /* TODO: hash by something */
}

void
Shader::load(String vertexPath, String fragmentPath)
{
    addToMap();
    loadVertFrag(this, vertexPath, fragmentPath);
}

void
Shader::load(String vertexPath, String geometryPath, String fragmentPath)
{
    addToMap();
    loadVertGeomFrag(this, vertexPath, geometryPath, fragmentPath);
}

static void
loadVertFrag(Shader* s, String vertexPath, String fragmentPath)
{
    GLint linked;
    GLuint vertex = ShaderLoadOne(GL_VERTEX_SHADER, vertexPath);
    GLuint fragment = ShaderLoadOne(GL_FRAGMENT_SHADER, fragmentPath);

    s->m_id = glCreateProgram();
    if (s->m_id == 0)
        LOG_FATAL("glCreateProgram failed: {}\n", s->m_id);

    glAttachShader(s->m_id, vertex);
    glAttachShader(s->m_id, fragment);

    glLinkProgram(s->m_id);
    glGetProgramiv(s->m_id, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        GLint infoLen = 0;
        char infoLog[255] {};
        glGetProgramiv(s->m_id, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1)
        {
            glGetProgramInfoLog(s->m_id, infoLen, nullptr, infoLog);
            LOG_FATAL("error linking program: {}\n", infoLog);
        }
        glDeleteProgram(s->m_id);
        LOG_FATAL("error linking program.\n");
    }

#ifndef NDEBUG
    glValidateProgram(s->m_id);
#endif

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    /*g_aAllShaders.getHandle(*s);*/
}

static void
loadVertGeomFrag(Shader* s, String vertexPath, String geometryPath, String fragmentPath)
{
    GLint linked;
    GLuint vertex = ShaderLoadOne(GL_VERTEX_SHADER, vertexPath);
    GLuint fragment = ShaderLoadOne(GL_FRAGMENT_SHADER, fragmentPath);
    GLuint geometry = ShaderLoadOne(GL_GEOMETRY_SHADER, geometryPath);

    s->m_id = glCreateProgram();
    if (s->m_id == 0)
        LOG_FATAL("glCreateProgram failed: {}\n", s->m_id);

    glAttachShader(s->m_id, vertex);
    glAttachShader(s->m_id, fragment);
    glAttachShader(s->m_id, geometry);

    glLinkProgram(s->m_id);
    glGetProgramiv(s->m_id, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        GLint infoLen = 0;
        glGetProgramiv(s->m_id, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1)
        {
            char infoLog[255] {};
            glGetProgramInfoLog(s->m_id, infoLen, nullptr, infoLog);
            LOG_FATAL("error linking program: {}\n", infoLog);
        }
        glDeleteProgram(s->m_id);
        LOG_FATAL("error linking program.\n");
    }

#ifndef NDEBUG
    glValidateProgram(s->m_id);
#endif

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    glDeleteShader(geometry);
}

void
Shader::destroy()
{
    if (m_id != 0)
    {
        glDeleteProgram(m_id);
        LOG_OK("Shader '{}' destroyed\n", m_id);
        m_id = 0;
    }
}

void
Shader::queryActiveUniforms()
{
    GLint maxUniformLen;
    GLint nUniforms;

    glGetProgramiv(m_id, GL_ACTIVE_UNIFORMS, &nUniforms);
    glGetProgramiv(m_id, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxUniformLen);

    char uniformName[255] {};
    LOG_OK("queryActiveUniforms for '{}':\n", m_id);

    for (int i = 0; i < nUniforms; i++)
    {
        GLint size;
        GLenum type;
        String typeName;

        glGetActiveUniform(m_id, i, maxUniformLen, nullptr, &size, &type, uniformName);
        switch (type)
        {
            case GL_FLOAT:
                typeName = "GL_FLOAT";
                break;

            case GL_FLOAT_VEC2:
                typeName = "GL_FLOAT_VEC2";
                break;

            case GL_FLOAT_VEC3:
                typeName = "GL_FLOAT_VEC3";
                break;

            case GL_FLOAT_VEC4:
                typeName = "GL_FLOAT_VEC4";
                break;

            case GL_FLOAT_MAT4:
                typeName = "GL_FLOAT_MAT4";
                break;

            case GL_FLOAT_MAT3:
                typeName = "GL_FLOAT_MAT3";
                break;

            case GL_SAMPLER_2D:
                typeName = "GL_SAMPLER_2D";
                break;

            default:
                typeName = "unknown";
                break;
        }
        LOG_OK("\tuniformName: '{}', type: '{}'\n", uniformName, typeName);
    }
}

static GLuint
ShaderLoadOne(GLenum type, String path)
{
    GLuint shader = glCreateShader(type);
    if (!shader)
        return 0;

    Arena al(SIZE_8K);
    defer( al.freeAll() );

    Opt<String> src = file::load(&al, path);
    if (!src) LOG_FATAL("error opening shader file: '{}'\n", path);

    const char* srcData = src.value().data();

    glShaderSource(shader, 1, &srcData, nullptr);
    glCompileShader(shader);

    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1)
        {
            char infoLog[255] {};
            glGetShaderInfoLog(shader, infoLen, nullptr, infoLog);
            LOG_FATAL("error compiling shader '{}'\n{}\n", path, infoLog);
        }
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}
