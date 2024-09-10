#include "Shader.hh"

#include "adt/Arena.hh"
#include "adt/file.hh"
#include "adt/logs.hh"

Array<Shader> g_aAllShaders;

static GLuint ShaderLoadOne(GLenum type, String path);

void
ShaderLoad(Shader* s, String vertexPath, String fragmentPath)
{
    GLint linked;
    GLuint vertex = ShaderLoadOne(GL_VERTEX_SHADER, vertexPath);
    GLuint fragment = ShaderLoadOne(GL_FRAGMENT_SHADER, fragmentPath);

    s->id = glCreateProgram();
    if (s->id == 0)
        LOG_FATAL("glCreateProgram failed: %d\n", s->id);

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
            LOG_FATAL("error linking program: %s\n", infoLog);
        }
        glDeleteProgram(s->id);
        LOG_FATAL("error linking program.\n");
    }

#ifdef DEBUG
    glValidateProgram(s->id);
#endif

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    ArrayPush(&g_aAllShaders, *s);
}

void
ShaderLoad(Shader* s, String vertexPath, String geometryPath, String fragmentPath)
{
    GLint linked;
    GLuint vertex = ShaderLoadOne(GL_VERTEX_SHADER, vertexPath);
    GLuint fragment = ShaderLoadOne(GL_FRAGMENT_SHADER, fragmentPath);
    GLuint geometry = ShaderLoadOne(GL_GEOMETRY_SHADER, geometryPath);

    s->id = glCreateProgram();
    if (s->id == 0)
        LOG_FATAL("glCreateProgram failed: %d\n", s->id);

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
            LOG_FATAL("error linking program: %s\n", infoLog);
        }
        glDeleteProgram(s->id);
        LOG_FATAL("error linking program.\n");
    }

#ifdef DEBUG
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
        LOG_OK("Shader '%d' destroyed\n", s->id);
        s->id = 0;
    }
}

void
ShaderUse(Shader* s)
{
    glUseProgram(s->id);
}

void
ShaderQueryActiveUniforms(Shader* s)
{
    GLint maxUniformLen;
    GLint nUniforms;

    glGetProgramiv(s->id, GL_ACTIVE_UNIFORMS, &nUniforms);
    glGetProgramiv(s->id, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxUniformLen);

    char uniformName[255] {};
    LOG_OK("queryActiveUniforms for '%d':\n", s->id);

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
        LOG_OK("\tuniformName: '%s', type: '%.*s'\n", uniformName, (int)typeName.size, typeName.pData);
    }
}

void 
ShaderSetM4(Shader* s, String name, const math::M4& m)
{
    GLint ul;
    ul = glGetUniformLocation(s->id, name.pData);
    glUniformMatrix4fv(ul, 1, GL_FALSE, (GLfloat*)m.e);
}

void 
ShaderSetM3(Shader* s, String name, const math::M3& m)
{
    GLint ul;
    ul = glGetUniformLocation(s->id, name.pData);
    glUniformMatrix3fv(ul, 1, GL_FALSE, (GLfloat*)m.e);
}

void
ShaderSetV3(Shader* s, String name, const math::V3& v)
{
    GLint ul;
    ul = glGetUniformLocation(s->id, name.pData);
    glUniform3fv(ul, 1, (GLfloat*)v.e);
}

void
ShaderSetI(Shader* s, String name, const GLint i)
{
    GLint ul;
    ul = glGetUniformLocation(s->id, name.pData);
    glUniform1i(ul, i);
}

void
ShaderSetF(Shader* s, String name, const f32 f)
{
    GLint ul;
    ul = glGetUniformLocation(s->id, name.pData);
    glUniform1f(ul, f);
}

static GLuint
ShaderLoadOne(GLenum type, String path)
{
    GLuint shader;
    shader = glCreateShader(type);
    if (!shader)
        return 0;

    Arena al(SIZE_8K);

    String src = file::load(&al.base, path);
    const char* srcData = src.pData;

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
            LOG_FATAL("error compiling shader '%.*s'\n%s\n", (int)path.size, path.pData, infoLog);
        }
        glDeleteShader(shader);
        return 0;
    }

    ArenaFreeAll(&al);
    return shader;
}
