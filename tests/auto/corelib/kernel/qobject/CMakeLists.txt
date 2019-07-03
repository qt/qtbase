# Generated from qobject.pro.

#####################################################################
## tst_qobject Test:
#####################################################################

add_qt_test(tst_qobject
    SOURCES
        tst_qobject.cpp
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Network
)

#### Keys ignored in scope 3:.:.:test.pro:<TRUE>:
# CONFIG = "testcase" "console"

## Scopes:
#####################################################################

#### Keys ignored in scope 4:.:.:test.pro:QT_CONFIG___contains___c++1z:
# CONFIG = "c++1z"

if(NOT WINRT)
    add_subdirectory(signalbug)
endif()
