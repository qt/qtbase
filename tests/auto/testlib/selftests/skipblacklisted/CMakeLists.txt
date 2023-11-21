#####################################################################
## skipblacklisted Binary:
#####################################################################

qt_internal_add_executable(skipblacklisted
    NO_INSTALL
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    SOURCES
        tst_skipblacklisted.cpp
    LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

qt_internal_apply_testlib_coverage_options(skipblacklisted)
