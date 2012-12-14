QT += network widgets

HEADERS       = blockingclient.h \
                fortunethread.h
SOURCES       = blockingclient.cpp \
                main.cpp \
                fortunethread.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/network/blockingfortuneclient
INSTALLS += target

