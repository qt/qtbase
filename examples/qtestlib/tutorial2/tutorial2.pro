SOURCES = testqstring.cpp
CONFIG  += qtestlib

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/qtestlib/tutorial2
sources.files = $$SOURCES *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/qtestlib/tutorial2
INSTALLS += target sources

symbian {
    TARGET.UID3 = 0xA000C60C
    include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
}
