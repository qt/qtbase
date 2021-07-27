
qt_internal_add_executable(junit
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_junit.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

qt_internal_apply_testlib_coverage_options(junit)
