# Generated from mouse.pro.

#####################################################################
## mouse Binary:
#####################################################################

qt_internal_add_executable(mouse
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
qt_internal_apply_testlib_coverage_options(mouse)
# special case end
