#include "gltf.hh"

#include "adt/file.hh"
#include "adt/logs.hh"

namespace gltf
{

static void ModelProcJSONObjs(Model* s);
static void ModelProcScenes(Model* s);
static void ModelProcBuffers(Model* s);
static void ModelProcBufferViews(Model* s);
static void ModelProcAccessors(Model* s);
static void ModelProcMeshes(Model* s);
static void ModelProcTexures(Model* s);
static void ModelProcMaterials(Model* s);
static void ModelProcImages(Model* s);
static void ModelProcNodes(Model* s);

enum class HASH_CODES : u64
{
    scene = hashFNV("scene"),
    scenes = hashFNV("scenes"),
    nodes = hashFNV("nodes"),
    meshes = hashFNV("meshes"),
    cameras = hashFNV("cameras"),
    buffers = hashFNV("buffers"),
    bufferViews = hashFNV("bufferViews"),
    accessors = hashFNV("accessors"),
    materials = hashFNV("materials"),
    textures = hashFNV("textures"),
    images = hashFNV("images"),
    samplers = hashFNV("samplers"),
    skins = hashFNV("skins"),
    animations = hashFNV("animations"),
    SCALAR = hashFNV("SCALAR"),
    VEC2 = hashFNV("VEC2"),
    VEC3 = hashFNV("VEC3"),
    VEC4 = hashFNV("VEC4"),
    MAT3 = hashFNV("MAT3"),
    MAT4 = hashFNV("MAT4")
};

#ifdef D_GLTF

static String
getTargetString(enum TARGET t)
{
    switch (t)
    {
        default:
        case TARGET::NONE:
            return "NONE";
        case TARGET::ARRAY_BUFFER:
            return "ARRAY_BUFFER";
        case TARGET::ELEMENT_ARRAY_BUFFER:
            return "ELEMENT_ARRAY_BUFFER";
    }
}

static String
accessorTypeToString(enum ACCESSOR_TYPE t)
{
    const char* ss[] {
        "SCALAR", "VEC2", "VEC3", "VEC4", /* MAT2, Unused */ "MAT3", "MAT4"
    };
    return ss[(int)(t)];
}

#endif

inline enum ACCESSOR_TYPE
stringToAccessorType(String sv)
{
    switch (hashFNV(sv))
    {
        default:
        case (u64)(HASH_CODES::SCALAR):
            return ACCESSOR_TYPE::SCALAR;
        case (u64)(HASH_CODES::VEC2):
            return ACCESSOR_TYPE::VEC2;
        case (u64)(HASH_CODES::VEC3):
            return ACCESSOR_TYPE::VEC3;
        case (u64)(HASH_CODES::VEC4):
            return ACCESSOR_TYPE::VEC4;
        case (u64)(HASH_CODES::MAT3):
            return ACCESSOR_TYPE::MAT3;
        case (u64)(HASH_CODES::MAT4):
            return ACCESSOR_TYPE::MAT4;
    }
}

inline union Type
assignUnionType(json::Object* obj, u32 n)
{
    auto& arr = json::getArray(obj);
    union Type type;

    for (u32 i = 0; i < n; i++)
        if (arr[i].tagVal.tag == json::TAG::LONG)
            type.MAT4.p[i] = f32(json::getLong(&arr[i]));
        else
            type.MAT4.p[i] = f32(json::getDouble(&arr[i]));

    return type;
}

static union Type
accessorTypeToUnionType(enum ACCESSOR_TYPE t, json::Object* obj)
{
    union Type type;

    switch (t)
    {
        default:
        case ACCESSOR_TYPE::SCALAR:
            {
                auto& arr = json::getArray(obj);
                if (arr[0].tagVal.tag == json::TAG::LONG)
                    type.SCALAR = f64(json::getLong(&arr[0]));
                else
                    type.SCALAR = f64(json::getDouble(&arr[0]));
            }
            break;
        case ACCESSOR_TYPE::VEC2:
            type = assignUnionType(obj, 2);
            break;
        case ACCESSOR_TYPE::VEC3:
            type = assignUnionType(obj, 3);
            break;
        case ACCESSOR_TYPE::VEC4:
            type = assignUnionType(obj, 4);
            break;
        case ACCESSOR_TYPE::MAT3:
            type = assignUnionType(obj, 3*3);
            break;
        case ACCESSOR_TYPE::MAT4:
            type = assignUnionType(obj, 4*4);
            break;
    }

    return type;
}

void
ModelLoad(Model* s, String path)
{
    json::ParserLoad(&s->parser, path);
    json::ParserParse(&s->parser);

    ModelProcJSONObjs(s);
    s->defaultSceneIdx = json::getLong(s->jsonObjs.scene);

    ModelProcScenes(s);
    ModelProcBuffers(s);
    ModelProcBufferViews(s);
    ModelProcAccessors(s);
    ModelProcMeshes(s);
    ModelProcTexures(s);
    ModelProcMaterials(s);
    ModelProcImages(s);
    ModelProcNodes(s);
}

void
ModelProcJSONObjs(Model* s)
{
    /* collect all the top level objects */
    for (auto& node : json::getObject(json::ParserGetHeadObj(&s->parser)))
    {
        switch (hashFNV(node.svKey))
        {
            default:
                break;
            case (u64)(HASH_CODES::scene):
                s->jsonObjs.scene = &node;
                break;
            case (u64)(HASH_CODES::scenes):
                s->jsonObjs.scenes = &node;
                break;
            case (u64)(HASH_CODES::nodes):
                s->jsonObjs.nodes = &node;
                break;
            case (u64)(HASH_CODES::meshes):
                s->jsonObjs.meshes = &node;
                break;
            case (u64)(HASH_CODES::cameras):
                s->jsonObjs.cameras = &node;
                break;
            case (u64)(HASH_CODES::buffers):
                s->jsonObjs.buffers = &node;
                break;
            case (u64)(HASH_CODES::bufferViews):
                s->jsonObjs.bufferViews = &node;
                break;
            case (u64)(HASH_CODES::accessors):
                s->jsonObjs.accessors = &node;
                break;
            case (u64)(HASH_CODES::materials):
                s->jsonObjs.materials = &node;
                break;
            case (u64)(HASH_CODES::textures):
                s->jsonObjs.textures = &node;
                break;
            case (u64)(HASH_CODES::images):
                s->jsonObjs.images = &node;
                break;
            case (u64)(HASH_CODES::samplers):
                s->jsonObjs.samplers = &node;
                break;
            case (u64)(HASH_CODES::skins):
                s->jsonObjs.skins = &node;
                break;
            case (u64)(HASH_CODES::animations):
                s->jsonObjs.animations = &node;
                break;
        }
    }

#ifdef D_GLTF
    LOG_OK("D_GLTF: '%.*s'\n", parser.sName.size, this->parser.sName.pData);
    auto check = [](String sv, json::Object* p) -> void {
        String s = p ? p->svKey : "(null)";
        CERR("\t%.*s: '%.*s'\n", sv.size, sv.data(), s.size, s.pData);
    };

    check("scene", jsonObjs.scene);
    check("scenes", jsonObjs.scenes);
    check("nodes", jsonObjs.nodes);
    check("meshes", jsonObjs.meshes);
    check("buffers", jsonObjs.buffers);
    check("bufferViews", jsonObjs.bufferViews);
    check("accessors", jsonObjs.accessors);
    check("materials", jsonObjs.materials);
    check("textures", jsonObjs.textures);
    check("images", jsonObjs.images);
    check("samplers", jsonObjs.samplers);
    check("skins", jsonObjs.skins);
    check("animations", jsonObjs.animations);
#endif
}

void
ModelProcScenes(Model* s)
{
    auto scenes = s->jsonObjs.scenes;
    auto& arr = json::getArray(scenes);
    for (auto& e : arr)
    {
        auto& obj = json::getObject(&e);
        auto pNodes = json::searchObject(obj, "nodes");
        if (pNodes)
        {
            auto& a = json::getArray(pNodes);
            for (auto& el : a)
                VecPush(&s->aScenes, s->pAl, {(u32)json::getLong(&el)});
        }
        else
        {
            VecPush(&s->aScenes, s->pAl, {0});
            break;
        }
    }
}

void
ModelProcBuffers(Model* s)
{
    auto buffers = s->jsonObjs.buffers;
    auto& arr = json::getArray(buffers);
    for (auto& e : arr)
    {
        auto& obj = json::getObject(&e);
        auto pByteLength = json::searchObject(obj, "byteLength");
        auto pUri = json::searchObject(obj, "uri");
        if (!pByteLength) LOG_FATAL("'byteLength' field is required\n");

        String svUri;
        Option<String> rsBin;
        String aBin;

        if (pUri)
        {
            svUri = json::getString(pUri);
            auto sNewPath = file::replacePathEnding(s->pAl, s->parser.sName, svUri);

            rsBin = file::load(s->pAl, sNewPath);
            if (!rsBin) LOG_WARN("error opening file: '%.*s'\n", sNewPath.size, sNewPath.pData);
            else
            {
                aBin = rsBin.data;
            }
        }

        VecPush(&s->aBuffers, s->pAl, {
            .byteLength = (u32)(json::getLong(pByteLength)),
            .uri = svUri,
            .aBin = aBin
        });
    }
}

void
ModelProcBufferViews(Model* s)
{
    auto bufferViews = s->jsonObjs.bufferViews;
    auto& arr = json::getArray(bufferViews);
    for (auto& e : arr)
    {
        auto& obj = json::getObject(&e);

        auto pBuffer = json::searchObject(obj, "buffer");
        if (!pBuffer) LOG_FATAL("'buffer' field is required\n");
        auto pByteOffset = json::searchObject(obj, "byteOffset");
        auto pByteLength = json::searchObject(obj, "byteLength");
        if (!pByteLength) LOG_FATAL("'byteLength' field is required\n");
        auto pByteStride = json::searchObject(obj, "byteStride");
        auto pTarget = json::searchObject(obj, "target");

        VecPush(&s->aBufferViews, s->pAl, {
            .buffer = (u32)(json::getLong(pBuffer)),
            .byteOffset = pByteOffset ? (u32)(json::getLong(pByteOffset)) : 0,
            .byteLength = (u32)(json::getLong(pByteLength)),
            .byteStride = pByteStride ? (u32)(json::getLong(pByteStride)) : 0,
            .target = pTarget ? (TARGET)(json::getLong(pTarget)) : TARGET::NONE
        });
    }
}

void
ModelProcAccessors(Model* s)
{
    auto accessors = s->jsonObjs.accessors;
    auto& arr = json::getArray(accessors);
    for (auto& e : arr)
    {
        auto& obj = json::getObject(&e);
 
        auto pBufferView = json::searchObject(obj, "bufferView");
        auto pByteOffset = json::searchObject(obj, "byteOffset");
        auto pComponentType = json::searchObject(obj, "componentType");
        if (!pComponentType) LOG_FATAL("'componentType' field is required\n");
        auto pCount = json::searchObject(obj, "count");
        if (!pCount) LOG_FATAL("'count' field is required\n");
        auto pMax = json::searchObject(obj, "max");
        auto pMin = json::searchObject(obj, "min");
        auto pType = json::searchObject(obj, "type");
        if (!pType) LOG_FATAL("'type' field is required\n");
 
        enum ACCESSOR_TYPE type = stringToAccessorType(json::getString(pType));
 
        VecPush(&s->aAccessors, s->pAl, {
            .bufferView = pBufferView ? (u32)(json::getLong(pBufferView)) : 0,
            .byteOffset = pByteOffset ? (u32)(json::getLong(pByteOffset)) : 0,
            .componentType = (COMPONENT_TYPE)(json::getLong(pComponentType)),
            .count = (u32)(json::getLong(pCount)),
            .max = pMax ? accessorTypeToUnionType(type, pMax) : Type{},
            .min = pMin ? accessorTypeToUnionType(type, pMin) : Type{},
            .type = type
        });
    }
}

void
ModelProcMeshes(Model* s)
{
    auto meshes = s->jsonObjs.meshes;
    auto& arr = json::getArray(meshes);
    for (auto& e : arr)
    {
        auto& obj = json::getObject(&e);
 
        auto pPrimitives = json::searchObject(obj, "primitives");
        if (!pPrimitives) LOG_FATAL("'primitives' field is required\n");
 
        VecBase<Primitive> aPrimitives(s->pAl);
        auto pName = json::searchObject(obj, "name");
        auto name = pName ? json::getString(pName) : "";
 
        auto& aPrim = json::getArray(pPrimitives);
        for (auto& p : aPrim)
        {
            auto& op = json::getObject(&p);
 
            auto pAttributes = json::searchObject(op, "attributes");
            auto& oAttr = json::getObject(pAttributes);
            auto pNORMAL = json::searchObject(oAttr, "NORMAL");
            auto pTANGENT = json::searchObject(oAttr, "TANGENT");
            auto pPOSITION = json::searchObject(oAttr, "POSITION");
            auto pTEXCOORD_0 = json::searchObject(oAttr, "TEXCOORD_0");
 
            auto pIndices = json::searchObject(op, "indices");
            auto pMode = json::searchObject(op, "mode");
            auto pMaterial = json::searchObject(op, "material");
 
            VecPush(&aPrimitives, s->pAl, {
                .attributes {
                    .NORMAL = pNORMAL ? static_cast<decltype(Primitive::attributes.NORMAL)>(json::getLong(pNORMAL)) : NPOS,
                    .POSITION = pPOSITION ? static_cast<decltype(Primitive::attributes.POSITION)>(json::getLong(pPOSITION)) : NPOS,
                    .TEXCOORD_0 = pTEXCOORD_0 ? static_cast<decltype(Primitive::attributes.TEXCOORD_0)>(json::getLong(pTEXCOORD_0)) : NPOS,
                    .TANGENT = pTANGENT ? static_cast<decltype(Primitive::attributes.TANGENT)>(json::getLong(pTANGENT)) : NPOS,
                },
                .indices = pIndices ? static_cast<decltype(Primitive::indices)>(json::getLong(pIndices)) : NPOS,
                .material = pMaterial ? static_cast<decltype(Primitive::material)>(json::getLong(pMaterial)) : NPOS,
                .mode = pMode ? static_cast<decltype(Primitive::mode)>(json::getLong(pMode)) : PRIMITIVES::TRIANGLES,
            });
        }
 
        VecPush(&s->aMeshes, s->pAl, {.aPrimitives = aPrimitives, .svName = name});
    }
}

void
ModelProcTexures(Model* s)
{
    auto textures = s->jsonObjs.textures;
    if (!textures) return;

    auto& arr = json::getArray(textures);
    for (auto& tex : arr)
    {
        auto& obj = json::getObject(&tex);

        auto pSource = json::searchObject(obj, "source");
        auto pSampler = json::searchObject(obj, "sampler");

        VecPush(&s->aTextures, s->pAl, {
            .source = pSource ? (u32)(json::getLong(pSource)) : NPOS,
            .sampler = pSampler ? (u32)(json::getLong(pSampler)) : NPOS
        });
    }
}

void
ModelProcMaterials(Model* s)
{
    auto materials = s->jsonObjs.materials;
    if (!materials) return;

    auto& arr = json::getArray(materials);
    for (auto& mat : arr)
    {
        auto& obj = json::getObject(&mat);

        TextureInfo texInfo {};

        auto pPbrMetallicRoughness = json::searchObject(obj, "pbrMetallicRoughness");
        if (pPbrMetallicRoughness)
        {
            auto& oPbr = json::getObject(pPbrMetallicRoughness);

            auto pBaseColorTexture = json::searchObject(oPbr, "baseColorTexture");
            if (pBaseColorTexture)
            {
                auto& objBct = json::getObject(pBaseColorTexture);

                auto pIndex = json::searchObject(objBct, "index");
                if (!pIndex) LOG_FATAL("index field is required\n");

                texInfo.index = json::getLong(pIndex);
            }
        }

        NormalTextureInfo normTexInfo {};

        auto pNormalTexture = json::searchObject(obj, "normalTexture");
        if (pNormalTexture)
        {
            auto& objNT = json::getObject(pNormalTexture);
            auto pIndex = json::searchObject(objNT, "index");
            if (!pIndex) LOG_FATAL("index filed is required\n");

            normTexInfo.index = json::getLong(pIndex);
        }

        VecPush(&s->aMaterials, s->pAl, {
            .pbrMetallicRoughness {
                .baseColorTexture = texInfo,
            },
            .normalTexture = normTexInfo
        });
    }
}

void
ModelProcImages(Model *s)
{
    auto imgs = s->jsonObjs.images;
    if (!imgs) return;

    auto& arr = json::getArray(imgs);
    for (auto& img : arr)
    {
        auto& obj = json::getObject(&img);

        auto pUri = json::searchObject(obj, "uri");
        if (pUri)
            VecPush(&s->aImages, s->pAl, {json::getString(pUri)});
    }
}

void
ModelProcNodes(Model* s)
{
    auto nodes = s->jsonObjs.nodes;
    auto& arr = json::getArray(nodes);
    for (auto& node : arr)
    {
        auto& obj = json::getObject(&node);

        Node nNode(s->pAl);

        auto pName = json::searchObject(obj, "name");
        if (pName) nNode.name = json::getString(pName);

        auto pCamera = json::searchObject(obj, "camera");
        if (pCamera) nNode.camera = (u32)(json::getLong(pCamera));

        auto pChildren = json::searchObject(obj, "children");
        if (pChildren)
        {
            auto& arrChil = json::getArray(pChildren);
            for (auto& c : arrChil)

            VecPush(&nNode.children, s->pAl, (u32)(json::getLong(&c)));
        }

        auto pMatrix = json::searchObject(obj, "matrix");
        if (pMatrix)
        {
            auto ut = assignUnionType(pMatrix, 4*4);
            nNode.matrix = ut.MAT4;
        }

        auto pMesh = json::searchObject(obj, "mesh");
        if (pMesh) nNode.mesh = (u32)(json::getLong(pMesh));

        auto pTranslation = json::searchObject(obj, "translation");
        if (pTranslation)
        {
            auto ut = assignUnionType(pTranslation, 3);
            nNode.translation = ut.VEC3;
        }

        auto pRotation = json::searchObject(obj, "rotation");
        if (pRotation)
        {
            auto ut = assignUnionType(pRotation, 4);
            nNode.rotation = ut.VEC4;
        }

        auto pScale = json::searchObject(obj, "scale");
        if (pScale)
        {
            auto ut = assignUnionType(pScale, 3);
            nNode.scale = ut.VEC3;
        }

        VecPush(&s->aNodes, s->pAl, nNode);
    }
}

} /* namespace gltf */
