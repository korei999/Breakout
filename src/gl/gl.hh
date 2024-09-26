#pragma once

#include <threads.h>

#ifdef __linux__
    #define GL_GLEXT_PROTOTYPES
    #include <GL/gl.h>
    #include <GL/glcorearb.h>
#elif _WIN32
    #include "../platform/win32/glad.h"
    #undef APIENTRY
#endif

#if 0

#ifdef DEBUG
    #define D(C)                                                                                                       \
        {                                                                                                              \
            /* call function C then check for an error, enabled with -DDEBUG and -DLOGS */                             \
            C;                                                                                                         \
            while ((glLastErrorCode = glGetError()))                                                                   \
            {                                                                                                          \
                switch (glLastErrorCode)                                                                               \
                {                                                                                                      \
                    default:                                                                                           \
                        LOG_WARN(WARNIN"unknown error: {:#x}\n", g_glLastErrorCode);                                   \
                        break;                                                                                         \
                                                                                                                       \
                    case 0x506:                                                                                        \
                        LOG_BAD("GL_INVALID_FRAMEBUFFER_OPERATION\n");                                                 \
                        break;                                                                                         \
                                                                                                                       \
                    case GL_INVALID_ENUM:                                                                              \
                        LOG_BAD(BA"GL_INVALID_ENUM\n");                                                                \
                        break;                                                                                         \
                                                                                                                       \
                    case GL_INVALID_VALUE:                                                                             \
                        LOG_BAD("GL_INVALID_VALUE\n");                                                                 \
                        break;                                                                                         \
                                                                                                                       \
                    case GL_INVALID_OPERATION:                                                                         \
                        LOG_BAD("GL_INVALID_OPERATION\n");                                                             \
                        break;                                                                                         \
                                                                                                                       \
                    case GL_OUT_OF_MEMORY:                                                                             \
                        LOG_BAD("GL_OUT_OF_MEMORY\n");                                                                 \
                        break;                                                                                         \
                }                                                                                                      \
            }                                                                                                          \
        }
#else
    #define D(C) C;
#endif

#endif

namespace gl
{

extern GLenum g_lastErrorCode;
extern mtx_t g_mtxGlContext;

void debugCallback(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* user
);

} /* namespace gl */
