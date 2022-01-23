#ifndef TEXTURE_H
#define TEXTURE_H

#include <QObject>
//#include <QOpenGLWidget>
//#include <QOpenGLFunctions>
//#include <QOpenGLVertexArrayObject>
//#include <QOpenGLBuffer>
#include "GL/glew.h"
#include "FreeImage.h"

class Texture : public QObject
{
    Q_OBJECT
private:
    explicit Texture(QObject *parent = nullptr);
public:
    static Texture* GetInstance();

    int LoadTextureFromFile(QString strFilename);

private:
    static Texture *m_sTexture;
signals:

};

#endif // TEXTURE_H
