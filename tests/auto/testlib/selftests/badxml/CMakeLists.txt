# Generated from badxml.pro.

#####################################################################
## badxml Binary:
#####################################################################

qt_internal_add_executable(badxml
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_badxml.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(badxml)
# special case end
