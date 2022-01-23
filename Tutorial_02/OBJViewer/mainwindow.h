#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "GL/glew.h"
#include "GL/wglew.h"
#include "glm/glm.hpp"
#include "glm/ext.hpp"

#include "radeon_rays.h"
#include "radeon_rays_cl.h"
#include "CLW.h"

#include "fmod.h"

#include "shader.h"
#include "model.h"

#include <QMainWindow>
#include <QTimer>
#include <QElapsedTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void InitGL(QWidget *pWidget);
    bool InitRadeonRays();
    void Init();
public slots:
    void Draw();
private:
    bool eventFilter(QObject *obj, QEvent *event);

    Ui::MainWindow *ui;

    // OpenGL
    HWND hWnd;
    HDC hDC;
    HGLRC hRC;
    // Shader
    GLuint m_program = 0;

    // RadeonRays
    CLWContext m_rtContext;
    CLWProgram m_rtProgram;
    RadeonRays::IntersectionApi *m_prtApi;
    CLWBuffer<Vertex> inVertex_buf;

    cl_mem ioVertex_clmem;
    CLWBuffer<Vertex> outVertex_buf_cl;
    CLWBuffer<RadeonRays::ray> ioRay_buf_cl;
    CLWBuffer<RadeonRays::Intersection> cl_intersection_buffer;
    RadeonRays::Buffer* ioRay_buffer;
    RadeonRays::Buffer* out_intersection_buffer;

    size_t nLocalSize = 1;
    int32_t nNumVertices = 0;
    size_t clWidth = 0;
    size_t clHeight = 0;

    // .OBJ file
    Model m_model;

    QTimer m_timer;

    // FPS
    QElapsedTimer m_elapsedTimer;
    uint64_t m_nElapsedTime;
    uint64_t m_nCurrentTime;
    float dt;
    int nFPS = 0;
    float fSec = 0;

    // FMOD Sound
    FMOD_SYSTEM *system;
    FMOD_SOUND *sound;
    FMOD_CHANNEL *channel;
};
#endif // MAINWINDOW_H
