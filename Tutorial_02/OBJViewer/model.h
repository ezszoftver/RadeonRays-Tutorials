#ifndef MODEL_H
#define MODEL_H

#include "GL/glew.h"
#include "GL/wglew.h"
#include "glm/glm.hpp"
#include "glm/ext.hpp"

#include <QObject>

typedef struct _Vertex
{
    glm::vec3 v3Position;
    glm::vec3 v3Normal;
    glm::vec2 v2TextCoord;
    glm::vec3 v3Color;
}
Vertex;

typedef struct _Material
{
    int m_nNumVertices = 0;
    std::vector< Vertex > m_listVertices;

    GLuint m_glTextureId = 0;

    // RadeonRays
    std::vector< uint32_t > m_listIndices;
    GLuint m_glIndexBuffer = 0;
}
Material;

class Model : public QObject
{
    Q_OBJECT
public:
    explicit Model(QObject *parent = nullptr);
    ~Model();

    void LoadFromOBJFile(QString strDir, QString strFilename);
    void Update(Vertex *pSrc);
    void Draw(GLuint program);
    void Release();

    std::vector< Vertex >& GetTessellationVertices();
    std::vector< glm::vec3 >& GetPositions();
    std::vector< int >& GetIndices();

    GLuint GetGLVertexBufferId();

private:
    void PreDraw();
    void ApplyTessellation(float fArea);
    void Tessellation(Vertex A, Vertex B, Vertex C, float fArea, QList< Vertex > &tessellationVertices);
    void GetCenter(Vertex &ret, Vertex A, Vertex B);
    float GetTriangleArea(glm::vec3 A, glm::vec3 B, glm::vec3 C);
    void CopyVerticesToAllVerticesList();
    void CreateOpenGLBuffers();

    std::vector< Material > m_listMaterials;

    // RadeonRays
    std::vector< Vertex > m_listTessellationVertices;
    GLuint m_glVertexBuffer = 0;
    int32_t m_nNumAllVertices = 0;

    std::vector< glm::vec3 > m_listPositions;
    std::vector< int > m_listIndices;
};

#endif // MODEL_H
