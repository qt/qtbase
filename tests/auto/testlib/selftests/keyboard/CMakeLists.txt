# Generated from keyboard.pro.

#####################################################################
## keyboard Binary:
#####################################################################

qt_internal_add_executable(keyboard
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_keyboard.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Test
        Qt::TestPrivate
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(keyboard)
# special case end
