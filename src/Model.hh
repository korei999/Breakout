#pragma once

#include "gltf/gltf.hh"
#include "math.hh"
#include "Shader.hh"
#include "Texture.hh"

#include <limits.h>

using namespace adt;

enum DRAW : int
{
    NONE     = 0,
    DIFF     = 1,      /* bind diffuse textures */
    NORM     = 1 << 1, /* bind normal textures */
    APPLY_TM = 1 << 2, /* apply transformation matrix */
    APPLY_NM = 1 << 3, /* generate and apply normal matrix */
    ALL      = INT_MAX
};

inline bool
operator&(enum DRAW l, enum DRAW r)
{
    return int(l) & int(r);
}

inline enum DRAW
operator|(enum DRAW l, enum DRAW r)
{
    return (enum DRAW)(int(l) | int(r));
}

inline enum DRAW
operator^(enum DRAW l, enum DRAW r)
{
    return (enum DRAW)((int)(l) ^ (int)(r));
}

struct Ubo
{
    GLuint id;
    u32 size;
    GLuint point;
};

void UboCreateBuffer(Ubo* s, u32 size, GLint drawMode);
void UboBindShader(Ubo *s, Shader* sh, String block, GLuint point);
void UboBufferData(Ubo* s, void* pData, u32 offset, u32 size);
void UboDestroy(Ubo* s);

struct Materials
{
    Texture diffuse;
    Texture normal;
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

    enum gltf::COMPONENT_TYPE indType;
    enum gltf::PRIMITIVES mode;
    u32 triangleCount;
};

struct Model
{
    Allocator* pAlloc;
    String sSavedPath;
    Vec<Vec<Mesh>> aaMeshes;
    gltf::Model modelData;
    Vec<int> aTmIdxs; /* parents map */
    Vec<int> aTmCounters; /* map's sizes */

    Model(Allocator* p) : pAlloc(p), aaMeshes(p), modelData(p), aTmIdxs(p), aTmCounters(p) {}

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
void ModelLoadGLTF(Model* s, String path, GLint drawMode, GLint texMode);
void ModelDraw(
    Model* s,
    enum DRAW flags,
    Shader* sh = nullptr,
    String svUniform = "",
    String svUniformM3Norm = "",
    const math::M4& tmGlobal = math::M4Iden()
);
void
ModelDrawGraph(
    Model* s,
    [[maybe_unused]] Allocator* pFrameAlloc,
    enum DRAW flags,
    Shader* sh,
    String svUniform,
    String svUniformM3Norm,
    const math::M4& tmGlobal
);

void PlainDraw(Plain* s);
void PlainDrawBox(Plain* s);
void PlainDestroy(Plain* s);
void QuadDraw(Quad* s);

struct ModelLoadArg
{
    Model* p;
    String path;
    GLint drawMode;
    GLint texMode;
};

inline int
ModelLoadSubmit(void* p)
{
    auto a = *(ModelLoadArg*)p;
    ModelLoad(a.p, a.path, a.drawMode, a.texMode);
    return 0;
};
