#####################################################################
## eventloop Binary:
#####################################################################

qt_internal_add_executable(eventloop
    NO_INSTALL
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    SOURCES
        tst_eventloop.cpp
    LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################
qt_internal_apply_testlib_coverage_options(eventloop)
