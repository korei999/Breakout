#include "Shader.hh"

#include "adt/Arena.hh"
#include "adt/file.hh"
#include "adt/logs.hh"

Pool<Shader, SHADER_MAX_COUNT> g_aAllShaders(INIT_FLAG::INIT);

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
ShaderLoad(Shader* s, String vertexPath, String fragmentPath)
{
    addToMap();
    loadVertFrag(s, vertexPath, fragmentPath);
}

void
ShaderLoad(Shader* s, String vertexPath, String geometryPath, String fragmentPath)
{
    addToMap();
    loadVertGeomFrag(s, vertexPath, geometryPath, fragmentPath);
}

static void
loadVertFrag(Shader* s, String vertexPath, String fragmentPath)
{
    GLint linked;
    GLuint vertex = ShaderLoadOne(GL_VERTEX_SHADER, vertexPath);
    GLuint fragment = ShaderLoadOne(GL_FRAGMENT_SHADER, fragmentPath);

    s->id = glCreateProgram();
    if (s->id == 0)
        LOG_FATAL("glCreateProgram failed: {}\n", s->id);

    glAttachShader(s->id, vertex);
    glAttachShader(s->id, fragment);

    glLinkProgram(s->id);
    glGetProgramiv(s->id, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        GLint infoLen = 0;
        char infoLog[255] {};
        glGetProgramiv(s->id, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1)
        {
            glGetProgramInfoLog(s->id, infoLen, nullptr, infoLog);
            LOG_FATAL("error linking program: {}\n", infoLog);
        }
        glDeleteProgram(s->id);
        LOG_FATAL("error linking program.\n");
    }

#ifndef NDEBUG
    glValidateProgram(s->id);
#endif

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    g_aAllShaders.getHandle(*s);
}

static void
loadVertGeomFrag(Shader* s, String vertexPath, String geometryPath, String fragmentPath)
{
    GLint linked;
    GLuint vertex = ShaderLoadOne(GL_VERTEX_SHADER, vertexPath);
    GLuint fragment = ShaderLoadOne(GL_FRAGMENT_SHADER, fragmentPath);
    GLuint geometry = ShaderLoadOne(GL_GEOMETRY_SHADER, geometryPath);

    s->id = glCreateProgram();
    if (s->id == 0)
        LOG_FATAL("glCreateProgram failed: {}\n", s->id);

    glAttachShader(s->id, vertex);
    glAttachShader(s->id, fragment);
    glAttachShader(s->id, geometry);

    glLinkProgram(s->id);
    glGetProgramiv(s->id, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        GLint infoLen = 0;
        glGetProgramiv(s->id, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1)
        {
            char infoLog[255] {};
            glGetProgramInfoLog(s->id, infoLen, nullptr, infoLog);
            LOG_FATAL("error linking program: {}\n", infoLog);
        }
        glDeleteProgram(s->id);
        LOG_FATAL("error linking program.\n");
    }

#ifndef NDEBUG
    glValidateProgram(s->id);
#endif

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    glDeleteShader(geometry);
}

void
ShaderDestroy(Shader* s)
{
    if (s->id != 0)
    {
        glDeleteProgram(s->id);
        LOG_OK("Shader '{}' destroyed\n", s->id);
        s->id = 0;
    }
}

void
ShaderQueryActiveUniforms(Shader* s)
{
    GLint maxUniformLen;
    GLint nUniforms;

    glGetProgramiv(s->id, GL_ACTIVE_UNIFORMS, &nUniforms);
    glGetProgramiv(s->id, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxUniformLen);

    char uniformName[255] {};
    LOG_OK("queryActiveUniforms for '{}':\n", s->id);

    for (int i = 0; i < nUniforms; i++)
    {
        GLint size;
        GLenum type;
        String typeName;

        glGetActiveUniform(s->id, i, maxUniformLen, nullptr, &size, &type, uniformName);
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

    const char* srcData = src.data.data();

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
