# Generated from tuplediagnostics.pro.

#####################################################################
## tuplediagnostics Binary:
#####################################################################

qt_add_executable(tuplediagnostics
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_tuplediagnostics.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

# special case begin
qt_apply_testlib_coverage_options(tuplediagnostics)
# special case end
