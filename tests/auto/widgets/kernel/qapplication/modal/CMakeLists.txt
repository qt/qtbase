# Generated from modal.pro.

#####################################################################
## modal Binary:
#####################################################################

qt_internal_add_executable(modal_helper  # special case
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
