HEADERS       = sender.h
SOURCES       = sender.cpp \
                main.cpp
QT           += network widgets
requires(qtConfig(udpsocket))

# install
target.path = $$[QT_INSTALL_EXAMPLES]/network/multicastsender
INSTALLS += target

