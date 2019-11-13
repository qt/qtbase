# Generated from modal.pro.

#####################################################################
## modal Binary:
#####################################################################

add_qt_executable(modal_helper  # special case
    SOURCES
        base.cpp base.h
        main.cpp
    OUTPUT_DIRECTORY # special case
        ${CMAKE_CURRENT_BINARY_DIR}/.. # special case
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
)

## Scopes:
#####################################################################
