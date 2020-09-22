# Generated from app.pro.

#####################################################################
## app Binary:
#####################################################################

qt_internal_add_executable(app
    GUI
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
)
