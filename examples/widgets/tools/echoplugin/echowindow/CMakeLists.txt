# Generated from echowindow.pro.

#####################################################################
## echoplugin Binary:
#####################################################################

add_qt_executable(echoplugin
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tools/echoplugin"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tools/echoplugin"
    SOURCES
        echointerface.h
        echowindow.cpp echowindow.h
        main.cpp
    LIBRARIES
        Qt::Widgets
)

#### Keys ignored in scope 1:.:echowindow.pro:<NONE>:
# CONFIG = "install_ok"
# QMAKE_PROJECT_NAME = "echopluginwindow"

## Scopes:
#####################################################################

#### Keys ignored in scope 3:.:echowindow.pro:CONFIG(debug,release OR debug):
# DESTDIR = "../debug/"

#### Keys ignored in scope 4:.:echowindow.pro:CONFIG(release,release OR debug):
# DESTDIR = "../release/"

#### Keys ignored in scope 5:.:echowindow.pro:else:
# DESTDIR = "../"
