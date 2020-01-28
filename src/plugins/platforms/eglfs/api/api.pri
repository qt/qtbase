
SOURCES +=  $$PWD/qeglfswindow.cpp \
            $$PWD/qeglfsscreen.cpp \
            $$PWD/qeglfshooks.cpp \
            $$PWD/qeglfsdeviceintegration.cpp \
            $$PWD/qeglfsintegration.cpp \
            $$PWD/qeglfsoffscreenwindow.cpp

HEADERS +=  $$PWD/qeglfswindow_p.h \
            $$PWD/qeglfsscreen_p.h \
            $$PWD/qeglfshooks_p.h \
            $$PWD/qeglfsdeviceintegration_p.h \
            $$PWD/qeglfsintegration_p.h \
            $$PWD/qeglfsoffscreenwindow_p.h \
            $$PWD/qeglfsglobal_p.h

qtConfig(opengl) {
    SOURCES += \
        $$PWD/qeglfscursor.cpp \
        $$PWD/qeglfscontext.cpp
    HEADERS += \
        $$PWD/qeglfscursor_p.h \
        $$PWD/qeglfscontext_p.h
}

qtConfig(vulkan) {
    SOURCES += \
        $$PWD/vulkan/qeglfsvulkaninstance.cpp \
        $$PWD/vulkan/qeglfsvulkanwindow.cpp
    HEADERS += \
        $$PWD/vulkan/qeglfsvulkaninstance_p.h \
        $$PWD/vulkan/qeglfsvulkanwindow_p.h
}

INCLUDEPATH += $$PWD
