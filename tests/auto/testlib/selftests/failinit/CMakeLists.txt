# Generated from failinit.pro.

#####################################################################
## failinit Binary:
#####################################################################

qt_internal_add_executable(failinit
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_failinit.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(failinit)
# special case end
