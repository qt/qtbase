# Generated from deform.pro.

#####################################################################
## deform Binary:
#####################################################################

add_qt_executable(deform
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/painting/deform"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/painting/deform"
    SOURCES
        main.cpp
        pathdeform.cpp pathdeform.h
    LIBRARIES # special case
        painting_shared
)

# Resources:
add_qt_resource(deform "deform" PREFIX "/res/deform" FILES
    pathdeform.cpp
    pathdeform.html)
