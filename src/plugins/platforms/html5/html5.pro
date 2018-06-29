TARGET = html5
CONFIG += static plugin
QT += \
    core-private gui-private \
    eventdispatcher_support-private fontdatabase_support-private egl_support-private

#DEFINES += QEGL_EXTRA_DEBUG

# Avoid X11 header collision, use generic EGL native types
DEFINES += QT_EGL_NO_X11

SOURCES =   main.cpp \
            qhtml5integration.cpp \
            qhtml5window.cpp \
            qhtml5screen.cpp \
            qhtml5fontdatabase.cpp \
            qhtml5eventtranslator.cpp \
            qhtml5eventdispatcher.cpp \
            qhtml5compositor.cpp \
            qhtml5cursor.cpp \
            qhtml5openglcontext.cpp \
            qhtml5theme.cpp

HEADERS =   qhtml5integration.h \
            qhtml5window.h \
            qhtml5screen.h \
            qhtml5fontdatabase.h \
            qhtml5eventtranslator.h \
            qhtml5eventdispatcher.h \
            qhtml5compositor.h \
            qhtml5stylepixmaps_p.h \
            qhtml5cursor.h \
            qhtml5openglcontext.h \
            qhtml5theme.h

RESOURCES += fonts/html5fonts.qrc

qtConfig(opengl) {
    SOURCES += qhtml5backingstore.cpp
    HEADERS += qhtml5backingstore.h
}
CONFIG +=  egl

OTHER_FILES += \
    html5.json \
    html5_shell.html \
    qtloader.js

shell_files.path = $$[QT_INSTALL_PLUGINS]/platforms
shell_files.files = html5_shell.html \
                    qtloader.js \
                    qtlogo.svg

INSTALLS += shell_files

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QHtml5IntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)
