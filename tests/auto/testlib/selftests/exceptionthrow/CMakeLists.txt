# Generated from exceptionthrow.pro.

#####################################################################
## exceptionthrow Binary:
#####################################################################

add_qt_executable(exceptionthrow
    EXCEPTIONS
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    GUI
    SOURCES
        tst_exceptionthrow.cpp
    PUBLIC_LIBRARIES
        Qt::Test
)

#### Keys ignored in scope 1:.:.:exceptionthrow.pro:<TRUE>:
# CONFIG = "-debug_and_release_target" "exceptions"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:exceptionthrow.pro:APPLE_OSX:
# CONFIG = "-app_bundle"
