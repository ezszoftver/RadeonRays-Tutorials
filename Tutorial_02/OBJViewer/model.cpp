#include "model.h"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

#include "texture.h"

Model::Model(QObject *parent) : QObject(parent)
{

}

Model::~Model()
{
    Release();
}

void Model::LoadFromOBJFile(QString strDir, QString strFilename)
{
    Assimp::Importer importer;
    const aiScene *pScene = importer.ReadFile((strDir + "/" + strFilename).toStdString(), aiProcessPreset_TargetRealtime_MaxQuality);

    for (uint m = 0; m < pScene->mNumMaterials; m++)
    {
        Material material;

        if (pScene->mMaterials[m]->GetTextureCount(aiTextureType_DIFFUSE) > 0)
        {
            aiString path;
            pScene->mMaterials[m]->GetTexture(aiTextureType_DIFFUSE, 0, &path);
            QString strTextureFilename = path.C_Str();

            material.m_glTextureId = Texture::GetInstance()->LoadTextureFromFile(strDir + "/" + strTextureFilename);
        }

        m_listMaterials.push_back(material);
    }

    for(uint m = 0; m < pScene->mNumMeshes; m++)
    {
        aiMesh *pMesh = pScene->mMeshes[m];

        Material &material = m_listMaterials[pMesh->mMaterialIndex];

        for(uint f = 0; f < pMesh->mNumFaces; f++)
        {
            aiFace face = pMesh->mFaces[f];

            for(int i = 0; i < 3; i++)
            {
                aiVector3D v = pMesh->mVertices[ face.mIndices[i] ];
                aiVector3D n = pMesh->mNormals[ face.mIndices[i] ];
                aiVector3D t = pMesh->mTextureCoords[0][ face.mIndices[i] ];

                Vertex vertex;
                vertex.v3Position = glm::vec3(v.x, v.y, v.z);
                vertex.v3Normal = glm::vec3(n.x, n.y, n.z);
                vertex.v2TextCoord = glm::vec2(t.x, t.y);
                vertex.v3Color = glm::vec3(1.0f, 1.0f, 1.0f);;

                material.m_listVertices.push_back(vertex);

                // radeonrays
                m_listPositions.push_back( glm::vec3(v.x, v.y, v.z) );
                int id = m_listIndices.size();
                m_listIndices.push_back( id );
            }
        }
    }

    // radeonrays
    float fEndArea = GetTriangleArea(glm::vec3(0,0,0), glm::vec3(0.2f, 0, 0), glm::vec3(0, 0.2f, 0));
    ApplyTessellation(fEndArea);

    CopyVerticesToAllVerticesList();

    CreateOpenGLBuffers();
}

void Model::CreateOpenGLBuffers()
{
    glGenBuffers(1, &m_glVertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_glVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * m_listTessellationVertices.size(), m_listTessellationVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    for(uint m = 0; m < m_listMaterials.size(); m++)
    {
        Material &material = m_listMaterials[m];

        if (material.m_listIndices.size() == 0)
        {
            continue;
        }

        glGenBuffers(1, &material.m_glIndexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, material.m_glIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * material.m_listIndices.size(), material.m_listIndices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
}

GLuint Model::GetGLVertexBufferId()
{
    return m_glVertexBuffer;
}

void Model::CopyVerticesToAllVerticesList()
{
    for(uint m = 0; m < m_listMaterials.size(); m++)
    {
        Material &material = m_listMaterials[m];

        for(uint i = 0; i < material.m_listVertices.size(); i++)
        {
            Vertex vertex = material.m_listVertices[i];
            m_listTessellationVertices.push_back(vertex);

            int32_t id = m_listTessellationVertices.size() - 1;
            material.m_listIndices.push_back(id);
        }
    }
}

void Model::ApplyTessellation(float fArea)
{
    for(uint m = 0; m < m_listMaterials.size(); m++)
    {
        Material &material = m_listMaterials[m];

        // tessenlation
        QList< Vertex > tessenlationVertices;
        for(uint i = 0; i < material.m_listVertices.size(); i += 3)
        {
            Vertex A = material.m_listVertices[i + 0];
            Vertex B = material.m_listVertices[i + 1];
            Vertex C = material.m_listVertices[i + 2];

            Tessellation(A, B, C, fArea, tessenlationVertices);
        }

        // copy
        material.m_listVertices.clear();
        for(int i = 0; i < tessenlationVertices.size(); i++)
        {
            material.m_listVertices.push_back(tessenlationVertices[i]);
        }
        tessenlationVertices.clear();
    }
}

void Model::Tessellation(Vertex A, Vertex B, Vertex C, float fArea, QList< Vertex > &tessellationVertices)
{
    float fTriangleArea = GetTriangleArea(A.v3Position, B.v3Position, C.v3Position);

    if (fArea < fTriangleArea)
    {
        Vertex D; GetCenter(D, A, B);
        Vertex E; GetCenter(E, B, C);
        Vertex F; GetCenter(F, C, A);

        Tessellation(A, D, F, fArea, tessellationVertices);
        Tessellation(D, E, F, fArea, tessellationVertices);
        Tessellation(D, B, E, fArea, tessellationVertices);
        Tessellation(F, E, C, fArea, tessellationVertices);
        return;
    }
    else
    {
        tessellationVertices.push_back(A);
        tessellationVertices.push_back(B);
        tessellationVertices.push_back(C);
    }
}

void Model::GetCenter(Vertex &ret, Vertex A, Vertex B)
{
    ret.v3Position = (A.v3Position * 0.5f) + (B.v3Position * 0.5f);
    ret.v3Normal = (A.v3Normal * 0.5f) + (B.v3Normal * 0.5f);
    ret.v2TextCoord = (A.v2TextCoord * 0.5f) + (B.v2TextCoord * 0.5f);
    ret.v3Color = (A.v3Color * 0.5f) + (B.v3Color * 0.5f);

    ret.v3Normal = glm::normalize(ret.v3Normal);
}

float Model::GetTriangleArea(glm::vec3 A, glm::vec3 B, glm::vec3 C)
{
    float fArea = glm::length( glm::cross(B - A, C - A) ) / 2.0f;
    return fArea;
}

void Model::Draw(GLuint program)
{
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glBindBuffer(GL_ARRAY_BUFFER, m_glVertexBuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(sizeof(glm::vec3)) );
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(sizeof(glm::vec3) + sizeof(glm::vec3)) );
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(sizeof(glm::vec3) + sizeof(glm::vec3) + sizeof(glm::vec2)) );

    GLuint texloc = glGetUniformLocation(program, "g_Texture");
    glUniform1i(texloc, 0);

    for(uint m = 0; m < m_listMaterials.size(); m++)
    {
        Material &material = m_listMaterials[m];

        if (material.m_listVertices.size() == 0)
        {
            continue;
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, material.m_glIndexBuffer);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, material.m_glTextureId);

        glDrawElements(GL_TRIANGLES, material.m_listIndices.size(), GL_UNSIGNED_INT, nullptr);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(3);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Model::Release()
{
    for(uint m = 0; m < m_listMaterials.size(); m++)
    {
        Material &material = m_listMaterials[m];

        material.m_listVertices.clear();

        if (0 != material.m_glTextureId)
        {
            glDeleteTextures(1, &material.m_glTextureId);
            material.m_glTextureId = 0;
        }
    }

    m_listMaterials.clear();
}

std::vector< Vertex >& Model::GetTessellationVertices()
{
    return m_listTessellationVertices;
}

std::vector< glm::vec3 >& Model::GetPositions()
{
    return m_listPositions;
}

std::vector< int >& Model::GetIndices()
{
    return m_listIndices;
}
