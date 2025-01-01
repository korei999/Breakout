#pragma once

#include "gltf/gltf.hh"
#include "adt/math.hh"
#include "adt/enum.hh"
#include "Shader.hh"
#include "texture.hh"

#include <limits.h>

enum DRAW : u32
{
    NONE     = 0,
    DIFF     = 1,      /* bind diffuse textures */
    NORM     = 1 << 1, /* bind normal textures */
    APPLY_TM = 1 << 2, /* apply transformation matrix */
    APPLY_NM = 1 << 3, /* generate and apply normal matrix */
    ALL      = NPOS
};
ADT_ENUM_BITWISE_OPERATORS(DRAW);

struct Ubo
{
    GLuint id;
    u32 size;
    GLuint point;

    void createBuffer(u32 size, GLint drawMode);
    void bindShader(Shader* sh, String block, GLuint point);
    void bufferData(void* pData, u32 offset, u32 size);
    void destroy();
};

struct Materials
{
    texture::Img diffuse;
    texture::Img normal;
};

struct MeshData
{
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLuint eboSize;

    Materials materials;
};

struct Mesh
{
    MeshData meshData;

    gltf::COMPONENT_TYPE indType;
    gltf::PRIMITIVES mode;
    u32 triangleCount;
};

struct Model
{
    IAllocator* m_pAlloc;
    String m_sSavedPath;
    VecBase<VecBase<Mesh>> m_aaMeshes;
    gltf::Model m_modelData;
    VecBase<int> m_aTmIdxs; /* parents map */
    VecBase<int> m_aTmCounters; /* map's sizes */

    /* */

    Model(IAllocator* p) : m_pAlloc(p), m_aaMeshes(p), m_modelData(p), m_aTmIdxs(p), m_aTmCounters(p) {}

    /* */

    void load(String path, GLint drawMode, GLint texMode);

    bool loadGLTF(String path, GLint drawMode, GLint texMode);

    void draw(
        DRAW flags,
        Shader* sh = nullptr,
        String svUniform = "",
        String svUniformM3Norm = "",
        const math::M4& tmGlobal = math::M4Iden()
    );

    void
    drawGraph(
        IAllocator* pFrameAlloc,
        DRAW flags,
        Shader* sh,
        String svUniform,
        String svUniformM3Norm,
        const math::M4& tmGlobal
    );
};

struct Quad
{
    GLuint m_vao;
    GLuint m_vbo;
    GLuint m_ebo;
    GLuint m_eboSize;

    /* */

    Quad() = default;
    Quad(GLint drawMode);

    /* */

    void draw();
};

struct Plain
{
    GLuint m_vao;
    GLuint m_vbo;

    /* */

    Plain() = default;
    Plain(GLint drawMode);

    /* */

    void draw();
    void drawBox();
    void destroy();
};

struct TextPlain
{
    GLuint m_vao;
    GLuint m_vbo;

    /* */

    TextPlain() = default;
    TextPlain(GLint drawMode);
};

struct ModelLoadArg
{
    Model* p;
    String path;
    GLint drawMode;
    GLint texMode;
};

inline int
ModelSubmit(void* p)
{
    auto a = *(ModelLoadArg*)p;
    a.p->load(a.path, a.drawMode, a.texMode);
    return 0;
};
