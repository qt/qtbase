# Generated from mouse.pro.

#####################################################################
## mouse Binary:
#####################################################################

add_qt_executable(mouse
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_mouse.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Test
        Qt::TestPrivate
)

## Scopes:
#####################################################################

# special case begin
qt_apply_testlib_coverage_options(mouse)
# special case end
