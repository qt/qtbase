# Generated from fetchbogus.pro.

#####################################################################
## fetchbogus Binary:
#####################################################################

qt_internal_add_executable(fetchbogus
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_fetchbogus.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(fetchbogus)
# special case end
