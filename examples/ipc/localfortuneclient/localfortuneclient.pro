HEADERS       = client.h
SOURCES       = client.cpp \
                main.cpp
QT           += network

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/ipc/localfortuneclient
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS localfortuneclient.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/ipc/localfortuneclient
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)


