#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QElapsedTimer>
#include <QDebug>

using namespace RadeonRays;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->view->installEventFilter(this);

    InitGL(ui->view);
    if (false == InitRadeonRays())
    {
        QApplication::exit(1);
        return;
    }

    Init();

    connect(&m_timer, SIGNAL(timeout()), this, SLOT(Draw()));
    m_timer.start(0);
}

MainWindow::~MainWindow()
{
    m_timer.stop();
    m_model.Release();
    delete ui;
}

void MainWindow::InitGL(QWidget *pWidget)
{
    hWnd = (HWND)pWidget->winId();
    hDC = GetDC(hWnd);

    PIXELFORMATDESCRIPTOR pfd =
    {
        sizeof(PIXELFORMATDESCRIPTOR),                  // Size Of This Pixel Format Descriptor
        1,                              // Version Number
        PFD_DRAW_TO_WINDOW |                        // Format Must Support Window
        PFD_SUPPORT_OPENGL |                        // Format Must Support OpenGL
        PFD_DOUBLEBUFFER,                       // Must Support Double Buffering
        PFD_TYPE_RGBA,                          // Request An RGBA Format
        32,                               // Select Our Color Depth
        0, 0, 0, 0, 0, 0,                       // Color Bits Ignored
        0,                              // No Alpha Buffer
        0,                              // Shift Bit Ignored
        0,                              // No Accumulation Buffer
        0, 0, 0, 0,                         // Accumulation Bits Ignored
        24,                             // Z-Buffer (Depth Buffer)
        8,                              // Stencil Buffer
        0,                              // No Auxiliary Buffer
        PFD_MAIN_PLANE,                         // Main Drawing Layer
        0,                              // Reserved
        0, 0, 0                             // Layer Masks Ignored
    };

    int pf = ChoosePixelFormat(hDC, &pfd);
    SetPixelFormat(hDC, pf, &pfd);
    hRC = wglCreateContext(hDC);
    wglMakeCurrent(hDC, hRC);

    glewInit();
}

bool MainWindow::InitRadeonRays()
{
    cl_int status = CL_SUCCESS;
    cl_uint numPlatforms;
    status = clGetPlatformIDs(0, nullptr, &numPlatforms);
    ThrowIf(status != CL_SUCCESS, status, "clGetPlatformIDs failed");

    std::vector<cl_platform_id> platformIds(numPlatforms);
    std::vector<cl_platform_id> validIds;

    status = clGetPlatformIDs(numPlatforms, &platformIds[0], nullptr);
    ThrowIf(status != CL_SUCCESS, status, "clGetPlatformIDs failed");

    cl_device_type type = CL_DEVICE_TYPE_GPU;

    // Only use CL1.2+ platforms
    for (auto & platformId : platformIds)
    {
        size_t size = 0;
        status = clGetPlatformInfo(platformId, CL_PLATFORM_VERSION, 0, nullptr, &size);

        std::vector<char> version(size);

        status = clGetPlatformInfo(platformId, CL_PLATFORM_VERSION, size, &version[0], nullptr);

        std::string versionstr(version.begin(), version.end());

        if (versionstr.find("OpenCL 1.0") != std::string::npos ||
            versionstr.find("OpenCL 1.1") != std::string::npos)
        {
            continue;
        }

        //validIds.push_back(platformId);
        CLWPlatform platform = CLWPlatform::Create(platformId, type);

        for (int d = 0; d < (int)platform.GetDeviceCount(); ++d)
        {
            if (platform.GetDevice(d).GetType() != CL_DEVICE_TYPE_GPU)
            {
                continue;
            }

            if (false == platform.GetDevice(d).HasGlInterop())
            {
                continue;
            }

            cl_context_properties props[ ] =
            {
            CL_GL_CONTEXT_KHR, (cl_context_properties) wglGetCurrentContext( ),
            CL_WGL_HDC_KHR, (cl_context_properties) wglGetCurrentDC( ),
            CL_CONTEXT_PLATFORM, (cl_context_properties) platformId,
            0
            };

            m_rtContext = CLWContext::Create(platform.GetDevice(d), props);

            m_rtProgram = CLWProgram::CreateFromFile("kernel.cl", " -cl-mad-enable -cl-fast-relaxed-math -cl-std=CL1.2 -I . ", m_rtContext);

            cl_device_id id = m_rtContext.GetDevice(0).GetID();
            cl_command_queue queue = m_rtContext.GetCommandQueue(0);

            // Create intersection API
            m_prtApi = RadeonRays::CreateFromOpenClContext(m_rtContext, id, queue);
            return true;
        }
    }

    return false;
}

void MainWindow::Init()
{
    wglMakeCurrent(hDC, hRC);

    //FMOD
    FMOD_System_Create(&system);
    FMOD_System_SetOutput(system, FMOD_OUTPUTTYPE_AUTODETECT);
    FMOD_System_Init(system, 50, FMOD_INIT_NORMAL, NULL);
    FMOD_System_CreateStream(system, "WINDWLKR.S3M", FMOD_2D | FMOD_LOOP_NORMAL, 0, &sound );
    FMOD_System_PlaySound(system, sound, 0, false, &channel);

    // opengl
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_CULL_FACE);
    //glEnable(GL_BLEND);
    wglSwapIntervalEXT(0);

    glClearColor(0.5f, 0.5f, 1.0f, 1.0f);

    m_program = Shader::Load("VertexShader.txt", "FragmentShader.txt");

    m_model.LoadFromOBJFile("OBJ", "Scene.obj");

    nNumVertices = m_model.GetTessellationVertices().size();
    clWidth  = (int32_t)std::ceil( std::sqrt(m_model.GetTessellationVertices().size()) );
    clHeight = (int32_t)std::ceil( std::sqrt(m_model.GetTessellationVertices().size()) );
    nLocalSize = 1;
    //m_model.GetTessellationVertices().resize(clWidth * clHeight);

    qDebug() << "Num Positions: " << m_model.GetPositions().size();
    qDebug() << "Num Tessellation Vertices: " << nNumVertices;

    inVertex_buf  = CLWBuffer<Vertex>::Create(m_rtContext, CL_MEM_READ_ONLY, m_model.GetTessellationVertices().size(), m_model.GetTessellationVertices().data());

    cl_int status = CL_SUCCESS;
    ioVertex_clmem = clCreateFromGLBuffer( m_rtContext, CL_MEM_READ_WRITE, m_model.GetGLVertexBufferId(), &status );
    ThrowIf(status != CL_SUCCESS, status, "clCreateBuffer failed");

    ioRay_buf_cl = CLWBuffer<ray>::Create(m_rtContext, CL_MEM_READ_WRITE, m_model.GetTessellationVertices().size());
    ioRay_buffer = CreateFromOpenClBuffer(m_prtApi, ioRay_buf_cl);

    cl_intersection_buffer = CLWBuffer<Intersection>::Create(m_rtContext, CL_MEM_READ_WRITE, m_model.GetTessellationVertices().size());
    out_intersection_buffer = CreateFromOpenClBuffer(m_prtApi, cl_intersection_buffer);

    // create radeon rays -> shape
    float* vertdata = (float*)m_model.GetPositions().data();
    int nvert = m_model.GetPositions().size();
    int* indices = m_model.GetIndices().data();
    int nfaces = m_model.GetIndices().size() / 3;
    Shape* shape = m_prtApi->CreateMesh(vertdata, nvert, 3 * sizeof(float), indices, 0, nullptr, nfaces);
    assert(shape != nullptr);
    m_prtApi->AttachShape(shape);
    shape->SetId(1);
    m_prtApi->Commit();

    m_elapsedTimer.start();
    m_nElapsedTime = m_nCurrentTime = m_elapsedTimer.nsecsElapsed();
}

void MainWindow::Draw()
{
    wglMakeCurrent(hDC, hRC);

    // getDT
    m_nElapsedTime = m_nCurrentTime;
    m_nCurrentTime = m_elapsedTimer.nsecsElapsed();
    dt = (float)(m_nCurrentTime - m_nElapsedTime) / 1000000000.0f;

    // print fps
    nFPS++;
    fSec += dt;
    if (fSec >= 1.0f)
    {
        this->setWindowTitle("FPS: " + QString::number(nFPS));

        nFPS = 0;
        fSec = 0.0f;
    }

    int nWidth = ui->view->width();
    int nHeight = ui->view->height();
    if (nWidth < 1) { nWidth = 1; }
    if (nHeight < 1) { nHeight = 1; }

    // update
    float fSpeed = 0.5f;
    static float fAngle = 0;
    fAngle += dt * fSpeed;
    glm::vec3 v3CameraPos = glm::vec3(-15 * cos(fAngle * 0.5f), 10, 15 * sin(fAngle * 0.5f));
    glm::vec3 v3CameraAt = glm::vec3(0, 1, 0);
    glm::vec3 v3CameraDir = glm::normalize(v3CameraAt - v3CameraPos);

    glm::mat4 mWorld = glm::transpose(glm::mat4(1.0f));
    glm::mat4 mView = glm::lookAtRH(v3CameraPos, v3CameraAt, glm::vec3(0, 1, 0));
    //glm::mat4 mModelView = mView * mWorld;
    glm::mat4 mProj = glm::perspective(3.1415f / 4.0f, (float)nWidth / (float)nHeight, 0.1f, 1000.0f);

    glm::vec3 v3DirLight = glm::normalize(glm::vec3(cos(fAngle), -1, sin(fAngle)));

    size_t x[] = { clWidth = (clHeight + nLocalSize - 1) / nLocalSize * nLocalSize, clHeight = (clHeight + nLocalSize - 1) / nLocalSize * nLocalSize };
    size_t y[] = { nLocalSize, nLocalSize };

    std::vector<cl_mem> objects;
    objects.push_back(ioVertex_clmem);

    m_rtContext.AcquireGLObjects(0, objects);

    CLWKernel kernelVertexShader = m_rtProgram.GetKernel("VertexShader");
    kernelVertexShader.SetArg(0, inVertex_buf);
    kernelVertexShader.SetArg(1, ioVertex_clmem);
    kernelVertexShader.SetArg(2, ioRay_buf_cl);
    kernelVertexShader.SetArg(3, (int32_t)nNumVertices);
    kernelVertexShader.SetArg(4, (int32_t)clWidth);
    kernelVertexShader.SetArg(5, (int32_t)clHeight);
    kernelVertexShader.SetArg(6, sizeof(glm::mat4), glm::value_ptr(mWorld));
    kernelVertexShader.SetArg(7, sizeof(glm::vec3), glm::value_ptr(v3CameraPos));
    kernelVertexShader.SetArg(8, sizeof(glm::vec3), glm::value_ptr(v3CameraAt));
    kernelVertexShader.SetArg(9, sizeof(glm::vec3), glm::value_ptr(v3DirLight));

    m_rtContext.Launch2D(0, x, y, kernelVertexShader);
    m_rtContext.Finish(0);

    m_rtContext.ReleaseGLObjects(0, objects);

    // shadow rays
    Event *e = nullptr;
    m_prtApi->QueryIntersection(ioRay_buffer, nNumVertices, out_intersection_buffer, nullptr, &e);
    e->Wait();
    m_prtApi->DeleteEvent(e);

    m_rtContext.AcquireGLObjects(0, objects);

    CLWKernel kernelShadowShader = m_rtProgram.GetKernel("ShadowShader");
    kernelShadowShader.SetArg(0, ioVertex_clmem);
    kernelShadowShader.SetArg(1, cl_intersection_buffer);
    kernelShadowShader.SetArg(2, (int32_t)nNumVertices);
    kernelShadowShader.SetArg(3, (int32_t)clWidth);
    kernelShadowShader.SetArg(4, (int32_t)clHeight);

    m_rtContext.Launch2D(0, x, y, kernelShadowShader);
    m_rtContext.Finish(0);

    m_rtContext.ReleaseGLObjects(0, objects);

    // draw
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //glMatrixMode(GL_PROJECTION);
    //glLoadMatrixf((float*)&mProj[0]);

    //glMatrixMode(GL_MODELVIEW);
    //glLoadMatrixf((float*)&mModelView[0]);

    glUseProgram(m_program);
    glm::mat4 mViewProj = glm::transpose(mProj * mView);
    GLuint matViewProj_loc = glGetUniformLocation(m_program, "matViewProj");
    glUniformMatrix4fv(matViewProj_loc, 1, GL_TRUE, glm::value_ptr(mViewProj));
    m_model.Draw(m_program);
    glUseProgram(0);

    SwapBuffers(hDC);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    bool ret = QObject::eventFilter(obj, event);

    if (event->type() == QEvent::Resize)
    {
        int nWidth = ui->view->width();
        int nHeight = ui->view->height();
        if (nWidth < 1) { nWidth = 1; }
        if (nHeight < 1) { nHeight = 1; }
        glViewport(0, 0, nWidth, nHeight);
    }
    return ret;
}
