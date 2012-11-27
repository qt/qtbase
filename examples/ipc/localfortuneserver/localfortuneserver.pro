HEADERS       = server.h
SOURCES       = server.cpp \
                main.cpp
QT           += network widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/ipc/localfortuneserver
INSTALLS += target


simulator: warning(This example might not fully work on Simulator platform)
