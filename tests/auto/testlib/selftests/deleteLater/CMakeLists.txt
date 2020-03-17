# Generated from deleteLater.pro.

#####################################################################
## deleteLater Binary:
#####################################################################

add_qt_executable(deleteLater
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        tst_deleteLater.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

# special case begin
qt_apply_testlib_coverage_options(deleteLater)
# special case end
