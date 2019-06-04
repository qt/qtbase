# Generated from watchdog.pro.

#####################################################################
## watchdog Binary:
#####################################################################

add_qt_executable(watchdog
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        tst_watchdog.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:watchdog.pro:<TRUE>:
# CONFIG = "-debug_and_release_target"
# QT_TOOL_ENV = "checkenv"
# checkenv.name = "QTEST_FUNCTION_TIMEOUT"
# checkenv.value = "100"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:watchdog.pro:APPLE:
# CONFIG = "-app_bundle"
