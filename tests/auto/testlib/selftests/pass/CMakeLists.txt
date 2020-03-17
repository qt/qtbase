# Generated from pass.pro.

#####################################################################
## pass Binary:
#####################################################################

qt_add_executable(pass
    NO_INSTALL # special case
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}" # special case
    SOURCES
        tst_pass.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_apply_testlib_coverage_options(pass)
# special case end
