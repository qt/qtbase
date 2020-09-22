# Generated from multiexec.pro.

#####################################################################
## multiexec Binary:
#####################################################################

qt_internal_add_executable(multiexec
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_multiexec.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(multiexec)
# special case end
