# Generated from failfetchtype.pro.

#####################################################################
## failfetchtype Binary:
#####################################################################

qt_internal_add_executable(failfetchtype
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_failfetchtype.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(failfetchtype)
# special case end
