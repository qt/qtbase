QT += network widgets

HEADERS       = dialog.h
SOURCES       = dialog.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/network/loopback
INSTALLS += target
