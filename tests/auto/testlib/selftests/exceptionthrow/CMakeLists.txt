# Generated from exceptionthrow.pro.

#####################################################################
## exceptionthrow Binary:
#####################################################################

qt_internal_add_executable(exceptionthrow
    EXCEPTIONS
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_exceptionthrow.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(exceptionthrow)
# special case end
