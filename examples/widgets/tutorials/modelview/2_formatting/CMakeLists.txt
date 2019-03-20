# Generated from 2_formatting.pro.

#####################################################################
## mv_formatting Binary:
#####################################################################

add_qt_executable(mv_formatting
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tutorials/modelview/2_formatting"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tutorials/modelview/2_formatting"
    SOURCES
        main.cpp
        mymodel.cpp mymodel.h
    LIBRARIES
        Qt::Widgets
)

#### Keys ignored in scope 1:.:2_formatting.pro:<NONE>:
# TEMPLATE = "app"
