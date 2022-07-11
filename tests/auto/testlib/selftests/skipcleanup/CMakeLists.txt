#####################################################################
## skipcleanup Binary:
#####################################################################

qt_internal_add_executable(skipcleanup
    NO_INSTALL
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    SOURCES
        tst_skipcleanup.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

qt_internal_apply_testlib_coverage_options(skipcleanup)
