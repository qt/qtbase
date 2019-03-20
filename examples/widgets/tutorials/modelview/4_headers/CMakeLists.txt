# Generated from 4_headers.pro.

#####################################################################
## mv_headers Binary:
#####################################################################

add_qt_executable(mv_headers
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tutorials/modelview/4_headers"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tutorials/modelview/4_headers"
    SOURCES
        main.cpp
        mymodel.cpp mymodel.h
    LIBRARIES
        Qt::Widgets
)

#### Keys ignored in scope 1:.:4_headers.pro:<NONE>:
# TEMPLATE = "app"
