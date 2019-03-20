# Generated from 3_changingmodel.pro.

#####################################################################
## mv_changingmodel Binary:
#####################################################################

add_qt_executable(mv_changingmodel
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tutorials/modelview/3_changingmodel"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/tutorials/modelview/3_changingmodel"
    SOURCES
        main.cpp
        mymodel.cpp mymodel.h
    LIBRARIES
        Qt::Widgets
)

#### Keys ignored in scope 1:.:3_changingmodel.pro:<NONE>:
# TEMPLATE = "app"
