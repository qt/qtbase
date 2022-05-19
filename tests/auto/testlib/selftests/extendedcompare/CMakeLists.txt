qt_internal_add_executable(extendedcompare
    NO_INSTALL
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    SOURCES
        tst_extendedcompare.cpp
    LIBRARIES
        Qt::Test
)

qt_internal_apply_testlib_coverage_options(extendedcompare)
