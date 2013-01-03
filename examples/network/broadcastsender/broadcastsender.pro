QT += network widgets

HEADERS       = sender.h
SOURCES       = sender.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/network/broadcastsender
INSTALLS += target
