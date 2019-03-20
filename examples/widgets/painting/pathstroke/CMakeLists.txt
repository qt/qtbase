# Generated from pathstroke.pro.

#####################################################################
## pathstroke Binary:
#####################################################################

add_qt_executable(pathstroke
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/painting/pathstroke"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/painting/pathstroke"
    SOURCES
        main.cpp
        pathstroke.cpp pathstroke.h
    LIBRARIES # special case
        painting_shared
)

# Resources:
add_qt_resource(pathstroke "pathstroke" PREFIX "/res/pathstroke" FILES
    pathstroke.cpp
    pathstroke.html)
