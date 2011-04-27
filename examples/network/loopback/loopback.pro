HEADERS       = dialog.h
SOURCES       = dialog.cpp \
                main.cpp
QT           += network

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/network/loopback
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS loopback.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/network/loopback
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
