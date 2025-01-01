#include "Model.hh"

#include "adt/MutexArena.hh"
#include "adt/ThreadPool.hh"
#include "adt/file.hh"
#include "adt/logs.hh"
#include "adt/utils.hh"
#include "app.hh"

void
ModelLoad(Model* s, String path, GLint drawMode, GLint texMode)
{
    if (path.endsWith(".gltf"))
        ModelLoadGLTF(s, path, drawMode, texMode);
    else
        LOG_FATAL("trying to load unsupported asset: '{}'\n", path);

    s->sSavedPath = path;
}

bool
ModelLoadGLTF(Model* s, String path, GLint drawMode, GLint texMode)
{
    if (!s->modelData.load(path)) return false;

    auto& a = s->modelData;;

    MutexArena atmAl(SIZE_1M * 10);
    defer( atmAl.freeAll() );

    /* load buffers first */
    Vec<GLuint> aBufferMap(&atmAl);
    for (u32 i = 0; i < a.m_aBuffers.getSize(); i++)
    {
        mtx_lock(&gl::g_mtxGlContext);
        app::g_pWindow->bindGlContext();
        defer(
            app::g_pWindow->unbindGlContext();
            mtx_unlock(&gl::g_mtxGlContext);
        );

        GLuint b;
        glGenBuffers(1, &b);
        glBindBuffer(GL_ARRAY_BUFFER, b);
        glBufferData(GL_ARRAY_BUFFER, a.m_aBuffers[i].byteLength, a.m_aBuffers[i].aBin.data(), drawMode);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        aBufferMap.push(b);
    }

    ThreadPool tp(&atmAl);
    tp.start();
    defer( tp.destroy() );

    /* preload texures */
    Vec<texture::Img> aTex(&atmAl, a.m_aImages.getSize());
    aTex.setSize(a.m_aImages.getCap());

    for (u32 i = 0; i < a.m_aImages.getSize(); i++)
    {
        auto uri = a.m_aImages[i].uri;

        if (!uri.endsWith(".bmp"))
            LOG_FATAL("trying to load unsupported texture: '{}'\n", uri);

        struct Args
        {
            texture::Img* p;
            IAllocator* pAlloc;
            String path;
            texture::TYPE type;
            bool flip;
            GLint texMode;
        };

        auto* arg = (Args*)atmAl.malloc(1, sizeof(Args));
        *arg = {
            .p = &aTex[i],
            .pAlloc = &atmAl,
            .path = file::replacePathEnding(s->pAlloc, path, uri),
            .type = texture::TYPE::DIFFUSE,
            .flip = true,
            .texMode = texMode
        };

        auto task = [](void* pArgs) -> int {
            auto a = *(Args*)pArgs;
            *a.p = texture::Img(a.pAlloc);
            a.p->load(a.path, a.flip, a.type, a.texMode);
            return 0;
        };

        tp.submit(task, arg);
    }

    tp.wait();

    for (auto& mesh : a.m_aMeshes)
    {
        VecBase<Mesh> aNMeshes(s->pAlloc);

        for (auto& primitive : mesh.aPrimitives)
        {
            u32 accIndIdx = primitive.indices;
            u32 accPosIdx = primitive.attributes.POSITION;
            u32 accNormIdx = primitive.attributes.NORMAL;
            u32 accTexIdx = primitive.attributes.TEXCOORD_0;
            u32 accTanIdx = primitive.attributes.TANGENT;
            u32 accMatIdx = primitive.material;
            gltf::PRIMITIVES mode = primitive.mode;

            auto& accPos = a.m_aAccessors[accPosIdx];
            auto& accTex = a.m_aAccessors[accTexIdx];

            auto& bvPos = a.m_aBufferViews[accPos.bufferView];
            auto& bvTex = a.m_aBufferViews[accTex.bufferView];

            Mesh nMesh {};

            nMesh.mode = mode;

            {
                mtx_lock(&gl::g_mtxGlContext);
                app::g_pWindow->bindGlContext();
                defer(
                    app::g_pWindow->unbindGlContext();
                    mtx_unlock(&gl::g_mtxGlContext);
                );

                glGenVertexArrays(1, &nMesh.meshData.vao);
                glBindVertexArray(nMesh.meshData.vao);

                if (accIndIdx != NPOS)
                {
                    auto& accInd = a.m_aAccessors[accIndIdx];
                    auto& bvInd = a.m_aBufferViews[accInd.bufferView];
                    nMesh.indType = accInd.componentType;
                    nMesh.meshData.eboSize = accInd.count;
                    nMesh.triangleCount = NPOS;

                    /* TODO: figure out how to reuse VBO data for index buffer (possible?) */
                    glGenBuffers(1, &nMesh.meshData.ebo);
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, nMesh.meshData.ebo);
                    glBufferData(
                        GL_ELEMENT_ARRAY_BUFFER, bvInd.byteLength,
                        &a.m_aBuffers[bvInd.buffer].aBin[bvInd.byteOffset + accInd.byteOffset], drawMode
                    );
                }
                else
                {
                    nMesh.triangleCount = accPos.count;
                }

                constexpr u32 v3Size = sizeof(math::V3) / sizeof(f32);
                constexpr u32 v2Size = sizeof(math::V2) / sizeof(f32);

                /* if there are different VBO's for positions textures or normals,
                 * given gltf file should be considered harmful, and this will crash ofc */
                nMesh.meshData.vbo = aBufferMap[bvPos.buffer];
                glBindBuffer(GL_ARRAY_BUFFER, nMesh.meshData.vbo);

                /* positions */
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(
                    0, v3Size, GLenum(accPos.componentType), GL_FALSE, bvPos.byteStride,
                    reinterpret_cast<void*>(bvPos.byteOffset + accPos.byteOffset)
                );

                /* texture coords */
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(
                    1, v2Size, GLenum(accTex.componentType), GL_FALSE, bvTex.byteStride,
                    reinterpret_cast<void*>(bvTex.byteOffset + accTex.byteOffset)
                );

                 /*normals */
                if (accNormIdx != NPOS)
                {
                    auto& accNorm = a.m_aAccessors[accNormIdx];
                    auto& bvNorm = a.m_aBufferViews[accNorm.bufferView];

                    glEnableVertexAttribArray(2);
                    glVertexAttribPointer(
                        2, v3Size, GLenum(accNorm.componentType), GL_FALSE, bvNorm.byteStride,
                        reinterpret_cast<void*>(accNorm.byteOffset + bvNorm.byteOffset)
                    );
                }

                /* tangents */
                if (accTanIdx != NPOS)
                {
                    auto& accTan = a.m_aAccessors[accTanIdx];
                    auto& bvTan = a.m_aBufferViews[accTan.bufferView];

                    glEnableVertexAttribArray(3);
                    glVertexAttribPointer(
                        3, v3Size, GLenum(accTan.componentType), GL_FALSE, bvTan.byteStride,
                        reinterpret_cast<void*>(accTan.byteOffset + bvTan.byteOffset)
                    );
                }

                glBindVertexArray(0);
            }

            /* load textures */
            if (accMatIdx != NPOS)
            {
                auto& mat = a.m_aMaterials[accMatIdx];
                u32 baseColorSourceIdx = mat.pbrMetallicRoughness.baseColorTexture.index;

                if (baseColorSourceIdx != NPOS)
                {
                    u32 diffTexInd = a.m_aTextures[baseColorSourceIdx].source;
                    if (diffTexInd != NPOS)
                    {
                        nMesh.meshData.materials.diffuse = aTex[diffTexInd];
                        nMesh.meshData.materials.diffuse.type = texture::TYPE::DIFFUSE;
                    }
                }

                u32 normalSourceIdx = mat.normalTexture.index;
                if (normalSourceIdx != NPOS)
                {
                    u32 normTexIdx = a.m_aTextures[normalSourceIdx].source;
                    if (normTexIdx != NPOS)
                    {
                        nMesh.meshData.materials.normal = aTex[normalSourceIdx];
                        nMesh.meshData.materials.normal.type = texture::TYPE::NORMAL;
                    }
                }
            }

            aNMeshes.push(s->pAlloc, nMesh);
        }
        s->aaMeshes.push(s->pAlloc, aNMeshes);
    }

    s->aTmIdxs = VecBase<int>(s->pAlloc, math::sq(s->modelData.m_aNodes.getSize()));
    s->aTmCounters = VecBase<int>(s->pAlloc, s->modelData.m_aNodes.getSize());
    s->aTmIdxs.setSize(s->pAlloc, math::sq(s->modelData.m_aNodes.getSize())); /* 2d map */
    s->aTmCounters.setSize(s->pAlloc, s->modelData.m_aNodes.getSize());

    auto& aNodes = s->modelData.m_aNodes;
    auto at = [&](int r, int c) -> int {
        return (r * aNodes.getSize()) + c;
    };

    for (int i = 0; i < (int)aNodes.getSize(); i++)
    {
        auto& node = aNodes[i];
        for (auto& ch : node.children)
            s->aTmIdxs[at(ch, s->aTmCounters[ch]++)] = i; /* give each children it's parent's idx's */
    }

    return true;
}

void
ModelDraw(Model* s, DRAW flags, Shader* sh, String svUniform, String svUniformM3Norm, const math::M4& tmGlobal)
{
    for (auto& m : s->aaMeshes)
    {
        for (auto& e : m)
        {
            glBindVertexArray(e.meshData.vao);

            if (flags & DRAW::DIFF)
                e.meshData.materials.diffuse.bind(GL_TEXTURE0);
            if (flags & DRAW::NORM)
                e.meshData.materials.normal.bind(GL_TEXTURE1);

            math::M4 m = math::M4Iden();
            if (flags & DRAW::APPLY_TM)
                m *= tmGlobal;

            if (sh)
            {
                sh->setM4(svUniform, m);
                if (flags & DRAW::APPLY_NM) sh->setM3(svUniformM3Norm, M3Normal(M4ToM3(m)));
            }

            if (e.triangleCount != NPOS)
                glDrawArrays(GLenum(e.mode), 0, e.triangleCount);
            else
            {
                glDrawElements(
                    GLenum(e.mode),
                    e.meshData.eboSize,
                    GLenum(e.indType),
                    nullptr
                );
            }
        }
    }
}

void
ModelDrawGraph(
    Model* s,
    [[maybe_unused]] IAllocator* pFrameAlloc,
    DRAW flags,
    Shader* sh,
    String svUniform,
    String svUniformM3Norm,
    const math::M4& tmGlobal
)
{
    auto& aNodes = s->modelData.m_aNodes;

    auto at = [&](int r, int c) -> int {
        return r*aNodes.getSize() + c;
    };

    for (int i = 0; i < (int)aNodes.getSize(); i++)
    {
        auto& node = aNodes[i];
        if (node.mesh != NPOS)
        {
            math::M4 tm = tmGlobal;
            math::Qt rot = math::QtIden();
            for (int j = 0; j < s->aTmCounters[i]; j++)
            {
                /* collect each transformation from parent's map */
                auto& n = aNodes[ s->aTmIdxs[at(i, j)] ];

                tm = M4Scale(tm, n.scale);
                rot *= n.rotation;
                tm *= n.matrix;
            }
            tm = M4Scale(tm, node.scale);
            tm *= QtRot(rot * node.rotation);
            tm = M4Translate(tm, node.translation);
            tm *= node.matrix;

            for (auto& e : s->aaMeshes[node.mesh])
            {
                glBindVertexArray(e.meshData.vao);

                if (flags & DRAW::DIFF)
                    e.meshData.materials.diffuse.bind(GL_TEXTURE0);
                if (flags & DRAW::NORM)
                    e.meshData.materials.normal.bind(GL_TEXTURE1);

                if (sh)
                {
                    sh->setM4(svUniform, tm);
                    if (flags & DRAW::APPLY_NM)
                        sh->setM3(svUniformM3Norm, M3Normal(M4ToM3(tm)));
                }

                if (e.triangleCount != NPOS)
                    glDrawArrays(GLenum(e.mode), 0, e.triangleCount);
                else
                {
                    glDrawElements(
                        GLenum(e.mode),
                        e.meshData.eboSize,
                        GLenum(e.indType),
                        nullptr
                    );
                }
            }
        }
    }
}

void
Ubo::createBuffer(u32 size, GLint drawMode)
{
    this->size = size;
    glGenBuffers(1, &this->id);
    glBindBuffer(GL_UNIFORM_BUFFER, this->id);
    glBufferData(GL_UNIFORM_BUFFER, size, nullptr, drawMode);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void
Ubo::bindShader(Shader* sh, String block, GLuint point)
{
    this->point = point;
    GLuint index = glGetUniformBlockIndex(sh->m_id, block.data());
    glUniformBlockBinding(sh->m_id, index, this->point);
    LOG_OK("uniform block: '{}' at '{}', in shader '{}'\n", block, index, sh->m_id);

    glBindBufferBase(GL_UNIFORM_BUFFER, point, this->id);
    /* or */
    /* glBindBufferRange(GL_UNIFORM_BUFFER, _point, id, 0, size); */
}

void
Ubo::bufferData(void* pData, u32 offset, u32 size)
{
    glBindBuffer(GL_UNIFORM_BUFFER, this->id);
    glBufferSubData(GL_UNIFORM_BUFFER, offset, size, pData);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void
Ubo::destroy()
{
    glDeleteBuffers(1, &this->id);
}

Quad::Quad(GLint drawMode)
{
    const f32 vertices[] {
        /* pos (4*3)*sizeof(f32) */
        -1.0f,  1.0f,  0.0f, /* tl */
        -1.0f, -1.0f,  0.0f, /* bl */
         1.0f, -1.0f,  0.0f, /* br */
         1.0f,  1.0f,  0.0f, /* tr */

         /* tex */
         0.0f,  1.0f,
         0.0f,  0.0f,
         1.0f,  0.0f,
         1.0f,  1.0f,    
    };


    const GLuint indices[] {
        0, 1, 2, 0, 2, 3
    };

    Quad q {};

    glGenVertexArrays(1, &q.m_vao);
    glBindVertexArray(q.m_vao);

    glGenBuffers(1, &q.m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, q.m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, drawMode);

    glGenBuffers(1, &q.m_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, q.m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, drawMode);
    q.m_eboSize = utils::size(indices);

    /* positions */
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    /* texture coords */
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(f32)*4*3));

    glBindVertexArray(0);

    LOG_OK("quad '{}' created\n", q.m_vao);
    *this = q;
}

void
Quad::draw()
{
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_eboSize, GL_UNSIGNED_INT, nullptr);
}

Plain::Plain(GLint drawMode)
{
    const f32 plainVertices[] {
         /* positions */       
        -1.0f + 1.0f,  1.0f + 1.0f, -0.0f, /* l-t */
        -1.0f + 1.0f, -1.0f + 1.0f, -0.0f, /* l-b */
         1.0f + 1.0f,  1.0f + 1.0f, -0.0f, /* r-t */

        -1.0f + 1.0f, -1.0f + 1.0f, -0.0f, /* l-b */
         1.0f + 1.0f, -1.0f + 1.0f, -0.0f, /* r-b */
         1.0f + 1.0f,  1.0f + 1.0f, -0.0f, /* r-t */

         /* texcoords    normals */
         0.0f, 1.0f,  0.0f, 1.0f, 0.0f,
         0.0f, 0.0f,  0.0f, 1.0f, 0.0f,
         1.0f, 1.0f,  0.0f, 1.0f, 0.0f,
                                       
         0.0f, 0.0f,  0.0f, 1.0f, 0.0f,
         1.0f, 0.0f,  0.0f, 1.0f, 0.0f,
         1.0f, 1.0f,  0.0f, 1.0f, 0.0f
    };

    Plain p;

    glGenVertexArrays(1, &p.m_vao);
    glBindVertexArray(p.m_vao);

    glGenBuffers(1, &p.m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, p.m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plainVertices), plainVertices, drawMode);

    constexpr u32 v3Size = sizeof(math::V3) / sizeof(f32);
    constexpr u32 v2Size = sizeof(math::V2) / sizeof(f32);
    constexpr u32 stride = 5 * sizeof(f32);
    /* positions */
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, v3Size, GL_FLOAT, GL_FALSE, 0, (void*)0);
    /* texture coords */
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, v2Size, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(f32) * 3*6));
    /* normals */
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, v3Size, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(f32) * (3*6 + v2Size)));

    glBindVertexArray(0);

    LOG_OK("plane '{}' created\n", p.m_vao);
    *this = p;
}

void
Plain::draw()
{
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void
Plain::drawBox()
{
    glBindVertexArray(m_vao);
    glDrawArrays(GL_LINE_LOOP, 0, 6);
}

void
Plain::destroy()
{
    glDeleteBuffers(1, &m_vao);
    /*glDeleteBuffers(1, &vbo);*/
    LOG_OK("plain {}(vao), {}(vbo) destroyed\n", m_vao, m_vbo);
}

TextPlain::TextPlain(GLint drawMode)
{
    Plain p;

    // glGenVertexArrays(1, &p.vao);
    // glBindVertexArray(p.vao);

    // glGenBuffers(1, &p.vbo);
    // glBindBuffer(GL_ARRAY_BUFFER, p.vbo);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(plainVertices), plainVertices, drawMode);

    // constexpr u32 v3Size = sizeof(math::V3) / sizeof(f32);
    // constexpr u32 v2Size = sizeof(math::V2) / sizeof(f32);
    // constexpr u32 stride = 5 * sizeof(f32);
    // /* positions */
    // glEnableVertexAttribArray(0);
    // glVertexAttribPointer(0, v3Size, GL_FLOAT, GL_FALSE, 0, (void*)0);
    // /* texture coords */
    // glEnableVertexAttribArray(1);
    // glVertexAttribPointer(1, v2Size, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(f32) * 3*6));
    // /* normals */
    // glEnableVertexAttribArray(2);
    // glVertexAttribPointer(2, v3Size, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(f32) * (3*6 + v2Size)));

    // glBindVertexArray(0);
}
