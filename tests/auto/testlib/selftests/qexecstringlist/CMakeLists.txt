# Generated from qexecstringlist.pro.

#####################################################################
## qexecstringlist Binary:
#####################################################################

qt_internal_add_executable(qexecstringlist
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_qexecstringlist.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(qexecstringlist)
# special case end
