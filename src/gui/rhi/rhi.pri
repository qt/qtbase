HEADERS += \
    rhi/qrhi_p.h \
    rhi/qrhi_p_p.h \
    rhi/qrhiprofiler_p.h \
    rhi/qrhiprofiler_p_p.h \
    rhi/qrhinull_p.h \
    rhi/qrhinull_p_p.h \
    rhi/qshader_p.h \
    rhi/qshader_p_p.h \
    rhi/qshaderdescription_p.h \
    rhi/qshaderdescription_p_p.h

SOURCES += \
    rhi/qrhi.cpp \
    rhi/qrhiprofiler.cpp \
    rhi/qrhinull.cpp \
    rhi/qshaderdescription.cpp \
    rhi/qshader.cpp

qtConfig(opengl) {
    HEADERS += \
        rhi/qrhigles2_p.h \
        rhi/qrhigles2_p_p.h
    SOURCES += \
        rhi/qrhigles2.cpp
}

qtConfig(vulkan) {
    HEADERS += \
        rhi/qrhivulkan_p.h \
        rhi/qrhivulkan_p_p.h
    SOURCES += \
        rhi/qrhivulkan.cpp
}

win32 {
    HEADERS += \
        rhi/qrhid3d11_p.h \
        rhi/qrhid3d11_p_p.h
    SOURCES += \
        rhi/qrhid3d11.cpp

    LIBS += -ld3d11 -ldxgi -ldxguid
}

macos|ios {
    HEADERS += \
        rhi/qrhimetal_p.h \
        rhi/qrhimetal_p_p.h
    SOURCES += \
        rhi/qrhimetal.mm

    macos: LIBS += -framework AppKit
    LIBS += -framework Metal
}

include($$PWD/../../3rdparty/VulkanMemoryAllocator.pri)
