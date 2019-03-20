# Generated from stylewindow.pro.

#####################################################################
## styleplugin Binary:
#####################################################################

add_qt_executable(styleplugin
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tools/styleplugin"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tools/styleplugin"
    SOURCES
        main.cpp
        stylewindow.cpp stylewindow.h
    LIBRARIES
        Qt::Widgets
)

#### Keys ignored in scope 1:.:stylewindow.pro:<NONE>:
# CONFIG = "install_ok"

## Scopes:
#####################################################################

#### Keys ignored in scope 3:.:stylewindow.pro:debug:
# DESTDIR = "../debug/"

#### Keys ignored in scope 4:.:stylewindow.pro:release:
# DESTDIR = "../release/"

#### Keys ignored in scope 5:.:stylewindow.pro:else:
# DESTDIR = "../"
