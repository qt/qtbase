# Generated from pairdiagnostics.pro.

#####################################################################
## pairdiagnostics Binary:
#####################################################################

add_qt_executable(pairdiagnostics
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        tst_pairdiagnostics.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:pairdiagnostics.pro:<TRUE>:
# CONFIG = "-app_bundle" "-debug_and_release_target"
