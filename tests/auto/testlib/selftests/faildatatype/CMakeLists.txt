# Generated from faildatatype.pro.

#####################################################################
## faildatatype Binary:
#####################################################################

qt_internal_add_executable(faildatatype
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_faildatatype.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(faildatatype)
# special case end
