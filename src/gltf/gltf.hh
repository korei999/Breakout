#pragma once

#include "json/Parser.hh"
#include "adt/math.hh"
#include "adt/String.hh"

using namespace adt;

namespace gltf
{

/* match gl macros */
enum class COMPONENT_TYPE
{
    BYTE = 5120,
    UNSIGNED_BYTE = 5121,
    SHORT = 5122,
    UNSIGNED_SHORT = 5123,
    UNSIGNED_INT = 5125,
    FLOAT = 5126
};

/* match gl macros */
enum class TARGET
{
    NONE = 0,
    ARRAY_BUFFER = 34962,
    ELEMENT_ARRAY_BUFFER = 34963
};

struct Scene
{
    u32 nodeIdx;
};

/* A buffer represents a block of raw binary data, without an inherent structure or meaning.
 * This data is referred to by a buffer using its uri.
 * This URI may either point to an external file, or be a data URI that encodes the binary data directly in the JSON file. */
struct Buffer
{
    u32 byteLength;
    String uri;
    String aBin;
};

enum class ACCESSOR_TYPE
{
    SCALAR,
    VEC2,
    VEC3,
    VEC4,
    /*MAT2, Unused*/
    MAT3,
    MAT4
};

union Type
{
    f64 SCALAR;
    math::V2 VEC2;
    math::V3 VEC3;
    math::V4 VEC4;
    /*m2 MAT2; Unused */
    math::M3 MAT3;
    math::M4 MAT4;
};

/* An accessor object refers to a bufferView and contains properties 
 * that define the type and layout of the data of the bufferView.
 * The raw data of a buffer is structured using bufferView objects and is augmented with data type information using accessor objects.*/
struct Accessor
{
    u32 bufferView;
    u32 byteOffset; /* The offset relative to the start of the buffer view in bytes. This MUST be a multiple of the size of the component datatype. */
    enum COMPONENT_TYPE componentType; /* REQUIRED */
    u32 count; /* REQUIRED The number of elements referenced by this accessor, not to be confused with the number of bytes or number of components. */
    union Type max;
    union Type min;
    enum ACCESSOR_TYPE type; /* REQUIRED */
};


/* Each node can contain an array called children that contains the indices of its child nodes.
 * So each node is one element of a hierarchy of nodes,
 * and together they define the structure of the scene as a scene graph. */
struct Node
{
    String name;
    u32 camera;
    VecBase<u32> children;
    math::M4 matrix = math::M4Iden();
    ssize mesh = NPOS; /* The index of the mesh in this node. */
    math::V3 translation {};
    math::V4 rotation = math::QtIden().base;
    math::V3 scale {1, 1, 1};

    Node() = default;
    Node(IAllocator* p) : children(p) {}
};

struct CameraPersp
{
    f64 aspectRatio;
    f64 yfov;
    f64 zfar;
    f64 znear;
};

struct CameraOrtho
{
    //
};

struct Camera
{
    union {
        CameraPersp perspective;
        CameraOrtho orthographic;
    } proj;
    enum {
        perspective,
        orthographic
    } type;
};

/* A bufferView represents a “slice” of the data of one buffer.
 * This slice is defined using an offset and a length, in bytes. */
struct BufferView
{
    u32 buffer;
    u32 byteOffset = 0; /* The offset into the buffer in bytes. */
    u32 byteLength;
    u32 byteStride = 0; /* The stride, in bytes, between vertex attributes. When this is not defined, data is tightly packed. */
    enum TARGET target;
};

struct Image
{
    String uri;
};

/* match real gl macros */
enum class PRIMITIVES
{
    POINTS = 0,
    LINES = 1,
    LINE_LOOP = 2,
    LINE_STRIP = 3,
    TRIANGLES = 4,
    TRIANGLE_STRIP = 5,
    TRIANGLE_FAN = 6
};

struct Primitive
{
    struct {
        u32 NORMAL = NPOS;
        u32 POSITION = NPOS;
        u32 TEXCOORD_0 = NPOS;
        u32 TANGENT = NPOS;
    } attributes; /* each value is the index of the accessor containing attribute’s data. */
    u32 indices = NPOS; /* The index of the accessor that contains the vertex indices, drawElements() when defined and drawArrays() otherwise. */
    u32 material = NPOS; /* The index of the material to apply to this primitive when rendering */
    enum PRIMITIVES mode = PRIMITIVES::TRIANGLES;
};

/* A mesh primitive defines the geometry data of the object using its attributes dictionary.
 * This geometry data is given by references to accessor objects that contain the data of vertex attributes. */
struct Mesh
{
    VecBase<Primitive> aPrimitives; /* REQUIRED */
    String svName;
};

struct Texture
{
    u32 source = NPOS; /* The index of the image used by this texture. */
    u32 sampler = NPOS; /* The index of the sampler used by this texture. When undefined, a sampler with repeat wrapping and auto filtering SHOULD be used. */
};

struct TextureInfo
{
    u32 index = NPOS; /* REQUIRED The index of the texture. */
};

struct NormalTextureInfo
{
    u32 index = NPOS; /* REQUIRED */
    f64 scale;
};

struct PbrMetallicRoughness
{
    TextureInfo baseColorTexture;
};

struct Material
{
    PbrMetallicRoughness pbrMetallicRoughness;
    NormalTextureInfo normalTexture;
};

struct Model
{
    IAllocator* m_pAlloc {};
    struct {
        json::Object* scene;
        json::Object* scenes;
        json::Object* nodes;
        json::Object* meshes;
        json::Object* cameras;
        json::Object* buffers;
        json::Object* bufferViews;
        json::Object* accessors;
        json::Object* materials;
        json::Object* textures;
        json::Object* images;
        json::Object* samplers;
        json::Object* skins;
        json::Object* animations;
    } m_jsonObjs {};
    json::Parser m_parser {};
    String m_sGenerator {};
    String m_sVersion {};
    u32 m_defaultSceneIdx {};
    VecBase<Scene> m_aScenes {};
    VecBase<Buffer> m_aBuffers {};
    VecBase<BufferView> m_aBufferViews {};
    VecBase<Accessor> m_aAccessors {};
    VecBase<Mesh> m_aMeshes {};
    VecBase<Texture> m_aTextures {};
    VecBase<Material> m_aMaterials {};
    VecBase<Image> m_aImages {};
    VecBase<Node> m_aNodes {};

    String m_sPath {};
    String m_sFile {};

    /* */

    Model(IAllocator* p)
        : m_pAlloc(p),
          m_aScenes(p),
          m_aBuffers(p),
          m_aBufferViews(p),
          m_aAccessors(p),
          m_aMeshes(p),
          m_aTextures(p),
          m_aMaterials(p),
          m_aImages(p),
          m_aNodes(p) {}

    /* */

    bool load(String path);

    /* */

private:
    void procJSONObjs();
    void procScenes();
    void procBuffers();
    void procBufferViews();
    void procAccessors();
    void procMeshes();
    void procTexures();
    void procMaterials();
    void procImages();
    void procNodes();
};

inline String
getComponentTypeString(enum COMPONENT_TYPE t)
{
    switch (t)
    {
        default:
        case COMPONENT_TYPE::BYTE:
            return "BYTE";
        case COMPONENT_TYPE::UNSIGNED_BYTE:
            return "UNSIGNED_BYTE";
        case COMPONENT_TYPE::SHORT:
            return "SHORT";
        case COMPONENT_TYPE::UNSIGNED_SHORT:
            return "UNSIGNED_SHORT";
        case COMPONENT_TYPE::UNSIGNED_INT:
            return "UNSIGNED_INT";
        case COMPONENT_TYPE::FLOAT:
            return "FLOAT";
    }
}

inline String
getPrimitiveModeString(enum PRIMITIVES pm)
{
    const char* ss[] {
        "POINTS", "LINES", "LINE_LOOP", "LINE_STRIP", "TRIANGLES", "TRIANGLE_STRIP", "TRIANGLE_FAN"
    };

    return ss[int(pm)];
}

} /* namespace gltf */
