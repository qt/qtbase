QT += network widgets

HEADERS       = receiver.h
SOURCES       = receiver.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/network/multicastreceiver
INSTALLS += target

