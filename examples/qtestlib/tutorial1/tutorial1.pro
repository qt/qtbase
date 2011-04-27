SOURCES = testqstring.cpp
CONFIG  += qtestlib

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/qtestlib/tutorial1
sources.files = $$SOURCES *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/qtestlib/tutorial1
INSTALLS += target sources

symbian {
    TARGET.UID3 = 0xA000C60B
    include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
}
