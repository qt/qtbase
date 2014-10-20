HEADERS       = client.h
SOURCES       = client.cpp \
                main.cpp
QT           += network widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/corelib/ipc/localfortuneclient
INSTALLS += target
