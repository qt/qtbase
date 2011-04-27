HEADERS       = receiver.h
SOURCES       = receiver.cpp \
                main.cpp
QT           += network

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/network/multicastreceiver
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS multicastreceiver.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/network/multicastreceiver
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
