# Generated from flowlayout.pro.

#####################################################################
## flowlayout Binary:
#####################################################################

add_qt_executable(flowlayout_layout # special case
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/layouts/flowlayout"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/layouts/flowlayout"
    SOURCES
        flowlayout.cpp flowlayout.h
        main.cpp
        window.cpp window.h
    LIBRARIES
        Qt::Widgets
)
