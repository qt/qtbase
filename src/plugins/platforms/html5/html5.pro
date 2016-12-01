TARGET = html5
CONFIG += static plugin
QT += \
    core-private gui-private \
    eventdispatcher_support-private fontdatabase_support-private egl_support-private

#DEFINES += QEGL_EXTRA_DEBUG

#DEFINES += Q_OPENKODE

# Avoid X11 header collision, use generic EGL native types
DEFINES += QT_EGL_NO_X11

SOURCES =   main.cpp \
            qhtml5integration.cpp \
            qhtml5window.cpp \
            qhtml5screen.cpp \
            qhtml5fontdatabase.cpp \
            qhtml5eventtranslator.cpp \
            qhtml5eventdispatcher.cpp \
            qhtml5compositor.cpp

HEADERS =   qhtml5integration.h \
            qhtml5window.h \
            qhtml5screen.h \
            qhtml5fontdatabase.h \
            qhtml5eventtranslator.h \
            qhtml5eventdispatcher.h \
            qhtml5compositor.h

RESOURCES += fonts/html5fonts.qrc

qtConfig(opengl) {
    SOURCES += qhtml5backingstore.cpp
    HEADERS += qhtml5backingstore.h
}
CONFIG +=  egl

OTHER_FILES += \
    html5.json \
    html5_shell.html

shell_file.path = $$[QT_INSTALL_PLUGINS]/platforms
shell_file.files = html5_shell.html
INSTALLS += shell_file

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QHTML5IntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)
