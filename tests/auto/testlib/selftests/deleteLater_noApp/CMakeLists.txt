# Generated from deleteLater_noApp.pro.

#####################################################################
## deleteLater_noApp Binary:
#####################################################################

qt_internal_add_executable(deleteLater_noApp
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_deleteLater_noApp.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

# special case begin
qt_internal_apply_testlib_coverage_options(deleteLater_noApp)
# special case end
