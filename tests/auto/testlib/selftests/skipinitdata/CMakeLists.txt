# Generated from skipinitdata.pro.

#####################################################################
## skipinitdata Binary:
#####################################################################

qt_add_executable(skipinitdata
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_skipinitdata.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_apply_testlib_coverage_options(skipinitdata)
# special case end
