# Generated from float.pro.

#####################################################################
## float Binary:
#####################################################################

add_qt_executable(float
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        ../../../../shared/emulationdetector.h
        tst_float.cpp
    INCLUDE_DIRECTORIES
        ../../../../shared
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_apply_testlib_coverage_options(float)
# special case end
