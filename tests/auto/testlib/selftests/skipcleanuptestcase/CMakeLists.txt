#####################################################################
## skipcleanuptestcase Binary:
#####################################################################

qt_internal_add_executable(skipcleanuptestcase
    NO_INSTALL
    EXCEPTIONS
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    SOURCES
        tst_skipcleanuptestcase.cpp
    LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

qt_internal_apply_testlib_coverage_options(skipcleanuptestcase)
