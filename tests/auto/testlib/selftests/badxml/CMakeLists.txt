# Generated from badxml.pro.

#####################################################################
## badxml Binary:
#####################################################################

qt_add_executable(badxml
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
qt_apply_testlib_coverage_options(badxml)
# special case end
