# Generated from blacklisted.pro.

#####################################################################
## blacklisted Binary:
#####################################################################

qt_internal_add_executable(blacklisted
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_blacklisted.cpp
    PUBLIC_LIBRARIES
        Qt::TestPrivate
)

## Scopes:
#####################################################################

# special case begin
qt_internal_apply_testlib_coverage_options(blacklisted)
# special case end
