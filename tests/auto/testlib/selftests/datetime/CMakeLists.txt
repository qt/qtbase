# Generated from datetime.pro.

#####################################################################
## datetime Binary:
#####################################################################

qt_internal_add_executable(datetime
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_datetime.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(datetime)
# special case end
