# Generated from cmptest.pro.

#####################################################################
## cmptest Binary:
#####################################################################

qt_add_executable(cmptest
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_cmptest.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

qt_extend_target(cmptest CONDITION TARGET Qt::Gui
    PUBLIC_LIBRARIES
        Qt::Gui
)

# special case begin
qt_apply_testlib_coverage_options(cmptest)
# special case end
