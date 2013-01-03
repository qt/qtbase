QT += network widgets

HEADERS       = dialog.h \
                fortuneserver.h \
                fortunethread.h
SOURCES       = dialog.cpp \
                fortuneserver.cpp \
                fortunethread.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/network/threadedfortuneserver
INSTALLS += target


