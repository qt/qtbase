# Generated from qvariant.pro.

#####################################################################
## tst_qvariant Test:
#####################################################################

add_qt_test(tst_qvariant
    GUI
    SOURCES
        tst_qvariant.cpp
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0
    INCLUDE_DIRECTORIES
        ../../../other/qvariant_common
    LIBRARIES
        Qt::CorePrivate
)

# Resources:
add_qt_resource(tst_qvariant "qvariant"
    PREFIX
        "/"
    FILES
        stream/qt4.9/
        stream/qt5.0/
)


#### Keys ignored in scope 1:.:.:qvariant.pro:<TRUE>:
# CONFIG = "testcase"

## Scopes:
#####################################################################

#### Keys ignored in scope 2:.:.:qvariant.pro:qtConfig(c++14):
# CONFIG = "c++14"

#### Keys ignored in scope 3:.:.:qvariant.pro:qtConfig(c++1z):
# CONFIG = "c++1z"
