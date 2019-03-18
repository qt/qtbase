# Generated from queuedcustomtype.pro.

#####################################################################
## queuedcustomtype Binary:
#####################################################################

add_qt_executable(queuedcustomtype
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/corelib/threads/queuedcustomtype"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/corelib/threads/queuedcustomtype"
    SOURCES
        block.cpp block.h
        main.cpp
        renderthread.cpp renderthread.h
        window.cpp window.h
    LIBRARIES
        Qt::Widgets
)
