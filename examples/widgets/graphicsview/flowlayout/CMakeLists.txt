# Generated from flowlayout.pro.

#####################################################################
## flowlayout Binary:
#####################################################################

add_qt_executable(flowlayout
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/graphicsview/flowlayout"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/graphicsview/flowlayout"
    SOURCES
        flowlayout.cpp flowlayout.h
        main.cpp
        window.cpp window.h
    LIBRARIES
        Qt::Widgets
)

#### Keys ignored in scope 1:.:flowlayout.pro:<NONE>:
# QMAKE_PROJECT_NAME = "flowlayout_graphicsview"
