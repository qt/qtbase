# Generated from differentexec.pro.

#####################################################################
## differentexec Binary:
#####################################################################

add_qt_executable(differentexec
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_differentexec.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

## Scopes:
#####################################################################

# special case begin
qt_apply_testlib_coverage_options(differentexec)
# special case end
