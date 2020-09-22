# Generated from datatable.pro.

#####################################################################
## datatable Binary:
#####################################################################

qt_internal_add_executable(datatable
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_datatable.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(datatable)
# special case end
