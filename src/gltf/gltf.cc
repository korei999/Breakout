#include "gltf.hh"

#include "adt/file.hh"
#include "adt/logs.hh"

namespace gltf
{

enum class HASH_CODES : u64
{
    scene = hash::func("scene"),
    scenes = hash::func("scenes"),
    nodes = hash::func("nodes"),
    meshes = hash::func("meshes"),
    cameras = hash::func("cameras"),
    buffers = hash::func("buffers"),
    bufferViews = hash::func("bufferViews"),
    accessors = hash::func("accessors"),
    materials = hash::func("materials"),
    textures = hash::func("textures"),
    images = hash::func("images"),
    samplers = hash::func("samplers"),
    skins = hash::func("skins"),
    animations = hash::func("animations"),
    SCALAR = hash::func("SCALAR"),
    VEC2 = hash::func("VEC2"),
    VEC3 = hash::func("VEC3"),
    VEC4 = hash::func("VEC4"),
    MAT3 = hash::func("MAT3"),
    MAT4 = hash::func("MAT4")
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

static enum ACCESSOR_TYPE
stringToAccessorType(String sv)
{
    switch (hash::func(sv))
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

static union Type
assignUnionType(json::Object* obj, u32 n)
{
    auto& arr = json::getArray(obj);
    union Type type;

    for (u32 i = 0; i < n; i++)
        if (arr[i].tagVal.tag == json::TAG::LONG)
            type.MAT4.d[i] = f32(json::getLong(&arr[i]));
        else
            type.MAT4.d[i] = f32(json::getDouble(&arr[i]));

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
            else type.SCALAR = f64(json::getDouble(&arr[0]));
        } break;

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

bool
Model::load(String path)
{
    auto o_sFile = file::load(m_pAlloc, path);
    if (!o_sFile) return false;

    m_sPath = path.clone(m_pAlloc);
    if (m_parser.parse(o_sFile.value()) == json::STATUS::FAIL) return false;

    procJSONObjs();
    m_defaultSceneIdx = json::getLong(m_jsonObjs.scene);

    procScenes();
    procBuffers();
    procBufferViews();
    procAccessors();
    procMeshes();
    procTexures();
    procMaterials();
    procImages();
    procNodes();

    return true;
}

void
Model::procJSONObjs()
{
    /* collect all the top level objects */
    for (auto& node : m_parser.getRoot())
    {
        switch (hash::func(node.sKey))
        {
            default: break;

            case (u64)(HASH_CODES::scene):
            m_jsonObjs.scene = &node;
            break;

            case (u64)(HASH_CODES::scenes):
            m_jsonObjs.scenes = &node;
            break;

            case (u64)(HASH_CODES::nodes):
            m_jsonObjs.nodes = &node;
            break;

            case (u64)(HASH_CODES::meshes):
            m_jsonObjs.meshes = &node;
            break;

            case (u64)(HASH_CODES::cameras):
            m_jsonObjs.cameras = &node;
            break;

            case (u64)(HASH_CODES::buffers):
            m_jsonObjs.buffers = &node;
            break;

            case (u64)(HASH_CODES::bufferViews):
            m_jsonObjs.bufferViews = &node;
            break;

            case (u64)(HASH_CODES::accessors):
            m_jsonObjs.accessors = &node;
            break;

            case (u64)(HASH_CODES::materials):
            m_jsonObjs.materials = &node;
            break;

            case (u64)(HASH_CODES::textures):
            m_jsonObjs.textures = &node;
            break;

            case (u64)(HASH_CODES::images):
            m_jsonObjs.images = &node;
            break;

            case (u64)(HASH_CODES::samplers):
            m_jsonObjs.samplers = &node;
            break;

            case (u64)(HASH_CODES::skins):
            m_jsonObjs.skins = &node;
            break;

            case (u64)(HASH_CODES::animations):
            m_jsonObjs.animations = &node;
            break;
        }
    }

#ifdef D_GLTF
    LOG_OK("D_GLTF: '{}'\n", thiparser.sName);
    auto check = [](String sv, json::Object* p) -> void {
        String s = p ? p->svKey : "(null)";
        CERR("\t{}: '{}'\n", sv, s);
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
Model::procScenes()
{
    auto scenes = m_jsonObjs.scenes;
    auto& arr = json::getArray(scenes);
    for (auto& e : arr)
    {
        auto& obj = json::getObject(&e);
        auto pNodes = json::searchObject(obj, "nodes");
        if (pNodes)
        {
            auto& a = json::getArray(pNodes);
            for (auto& el : a)
                m_aScenes.push(m_pAlloc, {(u32)json::getLong(&el)});
        }
        else
        {
            m_aScenes.push(m_pAlloc, {0});
            break;
        }
    }
}

void
Model::procBuffers()
{
    auto buffers = m_jsonObjs.buffers;
    auto& arr = json::getArray(buffers);
    for (auto& e : arr)
    {
        auto& obj = json::getObject(&e);
        auto pByteLength = json::searchObject(obj, "byteLength");
        auto pUri = json::searchObject(obj, "uri");
        if (!pByteLength) LOG_FATAL("'byteLength' field is required\n");

        String svUri;
        Opt<String> rsBin;
        String aBin;

        if (pUri)
        {
            svUri = json::getString(pUri);
            auto sNewPath = file::replacePathEnding(m_pAlloc, m_sPath, svUri);

            rsBin = file::load(m_pAlloc, sNewPath);
            if (!rsBin) LOG_WARN("error opening file: '{}'\n", sNewPath);
            else
            {
                aBin = rsBin.data;
            }
        }

        m_aBuffers.push(m_pAlloc, {
            .byteLength = (u32)(json::getLong(pByteLength)),
            .uri = svUri,
            .aBin = aBin
        });
    }
}

void
Model::procBufferViews()
{
    auto bufferViews = m_jsonObjs.bufferViews;
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

        m_aBufferViews.push(m_pAlloc, {
            .buffer = (u32)(json::getLong(pBuffer)),
            .byteOffset = pByteOffset ? (u32)(json::getLong(pByteOffset)) : 0,
            .byteLength = (u32)(json::getLong(pByteLength)),
            .byteStride = pByteStride ? (u32)(json::getLong(pByteStride)) : 0,
            .target = pTarget ? (TARGET)(json::getLong(pTarget)) : TARGET::NONE
        });
    }
}

void
Model::procAccessors()
{
    auto accessors = m_jsonObjs.accessors;
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
 
        m_aAccessors.push(m_pAlloc, {
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
Model::procMeshes()
{
    auto meshes = m_jsonObjs.meshes;
    auto& arr = json::getArray(meshes);
    for (auto& e : arr)
    {
        auto& obj = json::getObject(&e);
 
        auto pPrimitives = json::searchObject(obj, "primitives");
        if (!pPrimitives) LOG_FATAL("'primitives' field is required\n");
 
        VecBase<Primitive> aPrimitives(m_pAlloc);
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
 
            aPrimitives.push(m_pAlloc, {
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
 
        m_aMeshes.push(m_pAlloc, {.aPrimitives = aPrimitives, .svName = name});
    }
}

void
Model::procTexures()
{
    auto textures = m_jsonObjs.textures;
    if (!textures) return;

    auto& arr = json::getArray(textures);
    for (auto& tex : arr)
    {
        auto& obj = json::getObject(&tex);

        auto pSource = json::searchObject(obj, "source");
        auto pSampler = json::searchObject(obj, "sampler");

        m_aTextures.push(m_pAlloc, {
            .source = pSource ? (u32)(json::getLong(pSource)) : NPOS,
            .sampler = pSampler ? (u32)(json::getLong(pSampler)) : NPOS
        });
    }
}

void
Model::procMaterials()
{
    auto materials = m_jsonObjs.materials;
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

        m_aMaterials.push(m_pAlloc, {
            .pbrMetallicRoughness {
                .baseColorTexture = texInfo,
            },
            .normalTexture = normTexInfo
        });
    }
}

void
Model::procImages()
{
    auto imgs = m_jsonObjs.images;
    if (!imgs) return;

    auto& arr = json::getArray(imgs);
    for (auto& img : arr)
    {
        auto& obj = json::getObject(&img);

        auto pUri = json::searchObject(obj, "uri");
        if (pUri)
            m_aImages.push(m_pAlloc, {json::getString(pUri)});
    }
}

void
Model::procNodes()
{
    auto nodes = m_jsonObjs.nodes;
    auto& arr = json::getArray(nodes);
    for (auto& node : arr)
    {
        auto& obj = json::getObject(&node);

        Node nNode(m_pAlloc);

        auto pName = json::searchObject(obj, "name");
        if (pName) nNode.name = json::getString(pName);

        auto pCamera = json::searchObject(obj, "camera");
        if (pCamera) nNode.camera = (u32)(json::getLong(pCamera));

        auto pChildren = json::searchObject(obj, "children");
        if (pChildren)
        {
            auto& arrChil = json::getArray(pChildren);
            for (auto& c : arrChil)

            nNode.children.push(m_pAlloc, (u32)(json::getLong(&c)));
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

        m_aNodes.push(m_pAlloc, nNode);
    }
}

} /* namespace gltf */
