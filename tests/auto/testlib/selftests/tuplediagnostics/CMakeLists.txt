# Generated from tuplediagnostics.pro.

#####################################################################
## tuplediagnostics Binary:
#####################################################################

add_qt_executable(tuplediagnostics
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        tst_tuplediagnostics.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:tuplediagnostics.pro:<TRUE>:
# CONFIG = "-app_bundle" "-debug_and_release_target"
