QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += USE_OPENCL

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    SDKs/RadeonRays/CLWCommandQueue.cpp \
    SDKs/RadeonRays/CLWContext.cpp \
    SDKs/RadeonRays/CLWDevice.cpp \
    SDKs/RadeonRays/CLWEvent.cpp \
    SDKs/RadeonRays/CLWImage2D.cpp \
    SDKs/RadeonRays/CLWKernel.cpp \
    SDKs/RadeonRays/CLWParallelPrimitives.cpp \
    SDKs/RadeonRays/CLWPlatform.cpp \
    SDKs/RadeonRays/CLWProgram.cpp \
    SDKs/RadeonRays/ParameterHolder.cpp \
    SDKs/RadeonRays/ReferenceCounter.cpp \
    SDKs/RadeonRays/calc.cpp \
    SDKs/RadeonRays/calc_clw.cpp \
    SDKs/RadeonRays/calc_vkw.cpp \
    SDKs/RadeonRays/device_clw.cpp \
    SDKs/RadeonRays/device_vkw.cpp \
    main.cpp \
    mainwindow.cpp \
    model.cpp \
    shader.cpp \
    texture.cpp

HEADERS += \
    SDKs/RadeonRays/CLW.h \
    SDKs/RadeonRays/CLWBuffer.h \
    SDKs/RadeonRays/CLWCommandQueue.h \
    SDKs/RadeonRays/CLWContext.h \
    SDKs/RadeonRays/CLWDevice.h \
    SDKs/RadeonRays/CLWEvent.h \
    SDKs/RadeonRays/CLWExcept.h \
    SDKs/RadeonRays/CLWImage2D.h \
    SDKs/RadeonRays/CLWKernel.h \
    SDKs/RadeonRays/CLWParallelPrimitives.h \
    SDKs/RadeonRays/CLWPlatform.h \
    SDKs/RadeonRays/CLWProgram.h \
    SDKs/RadeonRays/ParameterHolder.h \
    SDKs/RadeonRays/ReferenceCounter.h \
    SDKs/RadeonRays/buffer.h \
    SDKs/RadeonRays/buffer_vk.h \
    SDKs/RadeonRays/calc.h \
    SDKs/RadeonRays/calc_cl.h \
    SDKs/RadeonRays/calc_clw.h \
    SDKs/RadeonRays/calc_clw_common.h \
    SDKs/RadeonRays/calc_common.h \
    SDKs/RadeonRays/calc_vk.h \
    SDKs/RadeonRays/calc_vkw.h \
    SDKs/RadeonRays/common_vk.h \
    SDKs/RadeonRays/device.h \
    SDKs/RadeonRays/device_cl.h \
    SDKs/RadeonRays/device_clw.h \
    SDKs/RadeonRays/device_vk.h \
    SDKs/RadeonRays/device_vkw.h \
    SDKs/RadeonRays/event.h \
    SDKs/RadeonRays/event_vk.h \
    SDKs/RadeonRays/except.h \
    SDKs/RadeonRays/except_clw.h \
    SDKs/RadeonRays/except_vk.h \
    SDKs/RadeonRays/executable.h \
    SDKs/RadeonRays/executable_vk.h \
    SDKs/RadeonRays/function_vk.h \
    SDKs/RadeonRays/kernelcache/clwkernels_cl.h \
    SDKs/RadeonRays/math/bbox.h \
    SDKs/RadeonRays/math/float2.h \
    SDKs/RadeonRays/math/float3.h \
    SDKs/RadeonRays/math/int2.h \
    SDKs/RadeonRays/math/int3.h \
    SDKs/RadeonRays/math/mathutils.h \
    SDKs/RadeonRays/math/matrix.h \
    SDKs/RadeonRays/math/quaternion.h \
    SDKs/RadeonRays/math/ray.h \
    SDKs/RadeonRays/primitives.h \
    SDKs/RadeonRays/radeon_rays.h \
    SDKs/RadeonRays/radeon_rays_cl.h \
    SDKs/RadeonRays/radeon_rays_vk.h \
    mainwindow.h \
    model.h \
    shader.h \
    texture.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32: LIBS += -L$$PWD/SDKs/Assimp/lib/ -lassimp-vc140-mt
INCLUDEPATH += $$PWD/SDKs/Assimp/include
DEPENDPATH += $$PWD/SDKs/Assimp/include

INCLUDEPATH += $$PWD/SDKs/glm/include
DEPENDPATH += $$PWD/SDKs/glm/include

win32: LIBS += -lkernel32 -luser32 -lgdi32 -lwinspool -lcomdlg32 -ladvapi32 -lshell32 -lole32 -loleaut32 -luuid -lodbc32 -lodbccp32
win32: LIBS += -lopengl32 -lglu32

win32: LIBS += -L$$PWD/SDKs/glew-2.1.0/lib/ -lglew32
INCLUDEPATH += $$PWD/SDKs/glew-2.1.0/include
DEPENDPATH += $$PWD/SDKs/glew-2.1.0/include

win32: LIBS += -L$$PWD/SDKs/FreeImage/lib/ -lFreeImage
INCLUDEPATH += $$PWD/SDKs/FreeImage/include
DEPENDPATH += $$PWD/SDKs/FreeImage/include

win32: LIBS += -L$$PWD/SDKs/OpenCL/lib/ -lOpenCL
INCLUDEPATH += $$PWD/SDKs/OpenCL/include
DEPENDPATH += $$PWD/SDKs/OpenCL/include

win32: LIBS += -L$$PWD/SDKs/RadeonRays/ -lRadeonRays
INCLUDEPATH += $$PWD/SDKs/RadeonRays
DEPENDPATH += $$PWD/SDKs/RadeonRays

win32: LIBS += -L$$PWD/SDKs/FMOD/lib/ -lfmod_vc
INCLUDEPATH += $$PWD/SDKs/FMOD/include
DEPENDPATH += $$PWD/SDKs/FMOD/include

DISTFILES += \
    SDKs/RadeonRays/CL/CLW.cl \
    SDKs/RadeonRays/CLW.lua \
    SDKs/RadeonRays/RadeonRays.lib \
    SDKs/RadeonRays/obj/x64/Release/CLW.log \
    SDKs/RadeonRays/obj/x64/Release/CLW.pdb \
    SDKs/RadeonRays/obj/x64/Release/CLW.tlog/CL.3412.write.1.tlog \
    SDKs/RadeonRays/obj/x64/Release/CLW.tlog/CL.command.1.tlog \
    SDKs/RadeonRays/obj/x64/Release/CLW.tlog/CL.read.1.tlog \
    SDKs/RadeonRays/obj/x64/Release/CLW.tlog/CLW.lastbuildstate \
    SDKs/RadeonRays/obj/x64/Release/CLW.tlog/Lib-link.read.1.tlog \
    SDKs/RadeonRays/obj/x64/Release/CLW.tlog/Lib-link.write.1.tlog \
    SDKs/RadeonRays/obj/x64/Release/CLW.tlog/Lib.command.1.tlog


