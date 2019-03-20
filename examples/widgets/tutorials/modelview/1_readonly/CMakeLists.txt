# Generated from 1_readonly.pro.

#####################################################################
## mv_readonly Binary:
#####################################################################

add_qt_executable(mv_readonly
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tutorials/modelview/1_readonly"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tutorials/modelview/1_readonly"
    SOURCES
        main.cpp
        mymodel.cpp mymodel.h
    LIBRARIES
        Qt::Widgets
)

#### Keys ignored in scope 1:.:1_readonly.pro:<NONE>:
# TEMPLATE = "app"
