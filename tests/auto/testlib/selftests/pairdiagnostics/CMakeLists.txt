# Generated from pairdiagnostics.pro.

#####################################################################
## pairdiagnostics Binary:
#####################################################################

qt_add_executable(pairdiagnostics
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_pairdiagnostics.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

# special case begin
qt_apply_testlib_coverage_options(pairdiagnostics)
# special case end
