# Generated from signaldumper.pro.

#####################################################################
## signaldumper Binary:
#####################################################################

qt_internal_add_executable(signaldumper
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_signaldumper.cpp
    PUBLIC_LIBRARIES
        Qt::TestPrivate
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(signaldumper)
# special case end
