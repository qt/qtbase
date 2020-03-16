# Generated from test.pro.

#####################################################################
## tst_qclipboard Test:
#####################################################################

add_qt_test(tst_qclipboard
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../"
    SOURCES
        ../tst_qclipboard.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
)

## Scopes:
#####################################################################

extend_target(tst_qclipboard CONDITION APPLE_MACOS
    PUBLIC_LIBRARIES
        ${FWAppKit}
)

#### Keys ignored in scope 6:.:.:test.pro:NOT ANDROID AND NOT WINRT:
# TEST_HELPER_INSTALLS = "../copier/copier" "../paster/paster"
