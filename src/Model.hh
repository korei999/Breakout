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
    IAllocator* pAlloc;
    String sSavedPath;
    VecBase<VecBase<Mesh>> aaMeshes;
    gltf::Model modelData;
    VecBase<int> aTmIdxs; /* parents map */
    VecBase<int> aTmCounters; /* map's sizes */

    Model(IAllocator* p) : pAlloc(p), aaMeshes(p), modelData(p), aTmIdxs(p), aTmCounters(p) {}
};

struct Quad
{
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLuint eboSize;

    Quad() = default;
    Quad(GLint drawMode);
};

struct Plain
{
    GLuint vao;
    GLuint vbo;

    Plain() = default;
    Plain(GLint drawMode);
};

void ModelLoad(Model* s, String path, GLint drawMode, GLint texMode);
bool ModelLoadGLTF(Model* s, String path, GLint drawMode, GLint texMode);
void ModelDraw(
    Model* s,
    DRAW flags,
    Shader* sh = nullptr,
    String svUniform = "",
    String svUniformM3Norm = "",
    const math::M4& tmGlobal = math::M4Iden()
);
void
ModelDrawGraph(
    Model* s,
    IAllocator* pFrameAlloc,
    DRAW flags,
    Shader* sh,
    String svUniform,
    String svUniformM3Norm,
    const math::M4& tmGlobal
);

void PlainDraw(Plain* s);
void PlainDrawBox(Plain* s);
void PlainDestroy(Plain* s);
void QuadDraw(Quad* s);

struct TextPlain
{
    GLuint vao;
    GLuint vbo;

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
    ModelLoad(a.p, a.path, a.drawMode, a.texMode);
    return 0;
};
