HEADERS       = server.h
SOURCES       = server.cpp \
                main.cpp
QT           += network

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/ipc/localfortuneserver
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS localfortuneserver.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/ipc/localfortuneserver
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)


