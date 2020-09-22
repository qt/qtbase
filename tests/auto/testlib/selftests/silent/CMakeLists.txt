# Generated from silent.pro.

#####################################################################
## silent Binary:
#####################################################################

qt_internal_add_executable(silent
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_silent.cpp
    PUBLIC_LIBRARIES
        Qt::TestPrivate
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(silent)
# special case end
