# Generated from testProcessEchoGui.pro.

#####################################################################
## testProcessEchoGui Binary:
#####################################################################

qt_add_executable(testProcessEchoGui
    GUI # special case
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0
    PUBLIC_LIBRARIES
        Qt::Gui
)

## Scopes:
#####################################################################

qt_extend_target(testProcessEchoGui CONDITION WIN32
    SOURCES
        main_win.cpp
    PUBLIC_LIBRARIES
        user32
)
