# Generated from affine.pro.

#####################################################################
## affine Binary:
#####################################################################

add_qt_executable(affine
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/painting/affine"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/painting/affine"
    SOURCES
        main.cpp
        xform.cpp xform.h
    # INCLUDE_DIRECTORIES ../shared # special case
    LIBRARIES # special case
        painting_shared
)

# Resources:
add_qt_resource(affine "affine" PREFIX "/res/affine" FILES
    bg1.jpg
    xform.cpp
    xform.html)
