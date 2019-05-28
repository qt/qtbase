# Note: OpenGL32 must precede Gdi32 as it overwrites some functions.
LIBS += -lwinspool -limm32 -loleaut32

QT_FOR_CONFIG += gui

qtConfig(opengl):!qtConfig(opengles2):!qtConfig(dynamicgl): LIBS *= -lopengl32

mingw: QMAKE_USE *= uuid
# For the dialog helpers:
LIBS += -lshlwapi -lwtsapi32

QMAKE_USE_PRIVATE += \
    advapi32 \
    d3d9/nolink \
    ole32 \
    shell32 \
    user32 \
    winmm

DEFINES *= QT_NO_CAST_FROM_ASCII QT_NO_FOREACH

SOURCES += \
    $$PWD/qwindowswindow.cpp \
    $$PWD/qwindowsintegration.cpp \
    $$PWD/qwindowscontext.cpp \
    $$PWD/qwindowsscreen.cpp \
    $$PWD/qwindowskeymapper.cpp \
    $$PWD/qwindowsmousehandler.cpp \
    $$PWD/qwindowspointerhandler.cpp \
    $$PWD/qwindowsole.cpp \
    $$PWD/qwindowsdropdataobject.cpp \
    $$PWD/qwindowsmime.cpp \
    $$PWD/qwindowsinternalmimedata.cpp \
    $$PWD/qwindowscursor.cpp \
    $$PWD/qwindowsinputcontext.cpp \
    $$PWD/qwindowstheme.cpp \
    $$PWD/qwindowsmenu.cpp \
    $$PWD/qwindowsdialoghelpers.cpp \
    $$PWD/qwindowsservices.cpp \
    $$PWD/qwindowsnativeinterface.cpp \
    $$PWD/qwindowsopengltester.cpp \
    $$PWD/qwin10helpers.cpp

HEADERS += \
    $$PWD/qwindowscombase.h \
    $$PWD/qwindowswindow.h \
    $$PWD/qwindowsintegration.h \
    $$PWD/qwindowscontext.h \
    $$PWD/qwindowsscreen.h \
    $$PWD/qwindowskeymapper.h \
    $$PWD/qwindowsmousehandler.h \
    $$PWD/qwindowspointerhandler.h \
    $$PWD/qtwindowsglobal.h \
    $$PWD/qwindowsole.h \
    $$PWD/qwindowsdropdataobject.h \
    $$PWD/qwindowsmime.h \
    $$PWD/qwindowsinternalmimedata.h \
    $$PWD/qwindowscursor.h \
    $$PWD/qwindowsinputcontext.h \
    $$PWD/qwindowstheme.h \
    $$PWD/qwindowsmenu.h \
    $$PWD/qwindowsdialoghelpers.h \
    $$PWD/qwindowsservices.h \
    $$PWD/qwindowsnativeinterface.h \
    $$PWD/qwindowsopengltester.h \
    $$PWD/qwindowsthreadpoolrunner.h \
    $$PWD/qwin10helpers.h

INCLUDEPATH += $$PWD

qtConfig(opengl): HEADERS += $$PWD/qwindowsopenglcontext.h

qtConfig(opengles2) {
    SOURCES += $$PWD/qwindowseglcontext.cpp
    HEADERS += $$PWD/qwindowseglcontext.h
} else: qtConfig(opengl) {
    SOURCES += $$PWD/qwindowsglcontext.cpp
    HEADERS += $$PWD/qwindowsglcontext.h
}

# Dynamic GL needs both WGL and EGL
qtConfig(dynamicgl) {
    SOURCES += $$PWD/qwindowseglcontext.cpp
    HEADERS += $$PWD/qwindowseglcontext.h
}

qtConfig(systemtrayicon) {
    SOURCES += $$PWD/qwindowssystemtrayicon.cpp
    HEADERS += $$PWD/qwindowssystemtrayicon.h
}

qtConfig(vulkan) {
    SOURCES += $$PWD/qwindowsvulkaninstance.cpp
    HEADERS += $$PWD/qwindowsvulkaninstance.h
}

qtConfig(clipboard) {
    SOURCES += $$PWD/qwindowsclipboard.cpp
    HEADERS += $$PWD/qwindowsclipboard.h
    # drag and drop on windows only works if a clipboard is available
    qtConfig(draganddrop) {
        HEADERS += $$PWD/qwindowsdrag.h
        SOURCES += $$PWD/qwindowsdrag.cpp
    }
}

qtConfig(tabletevent) {
    INCLUDEPATH += $$QT_SOURCE_TREE/src/3rdparty/wintab
    HEADERS += $$PWD/qwindowstabletsupport.h
    SOURCES += $$PWD/qwindowstabletsupport.cpp
}

qtConfig(sessionmanager) {
    SOURCES += $$PWD/qwindowssessionmanager.cpp
    HEADERS += $$PWD/qwindowssessionmanager.h
}

qtConfig(imageformat_png):RESOURCES += $$PWD/cursors.qrc

RESOURCES += $$PWD/openglblacklists.qrc

qtConfig(accessibility): include($$PWD/uiautomation/uiautomation.pri)

qtConfig(combined-angle-lib) {
    DEFINES *= LIBEGL_NAME=$${LIBQTANGLE_NAME}
    DEFINES *= LIBGLESV2_NAME=$${LIBQTANGLE_NAME}
} else {
    DEFINES *= LIBEGL_NAME=$${LIBEGL_NAME}
    DEFINES *= LIBGLESV2_NAME=$${LIBGLESV2_NAME}
}
