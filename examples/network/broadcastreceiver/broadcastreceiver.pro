QT += network widgets
requires(qtConfig(udpsocket))

HEADERS       = receiver.h
SOURCES       = receiver.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/network/broadcastreceiver
INSTALLS += target

