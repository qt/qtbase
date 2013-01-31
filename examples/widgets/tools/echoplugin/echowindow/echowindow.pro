QT += widgets

HEADERS    = echowindow.h \
             echointerface.h
SOURCES    = echowindow.cpp \
             main.cpp

TARGET     = echoplugin
QMAKE_PROJECT_NAME = echopluginwindow
win32 {
    CONFIG(debug, release|debug):DESTDIR = ../debug/
    CONFIG(release, release|debug):DESTDIR = ../release/
} else {
    DESTDIR    = ../
}

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tools/echoplugin
INSTALLS += target

simulator: warning(This example might not fully work on Simulator platform)
