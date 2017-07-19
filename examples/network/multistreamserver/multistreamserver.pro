QT += network widgets

HEADERS       = server.h \
                provider.h \
                movieprovider.h \
                timeprovider.h \
                chatprovider.h
SOURCES       = server.cpp \
                movieprovider.cpp \
                timeprovider.cpp \
                chatprovider.cpp \
                main.cpp

EXAMPLE_FILES = animation.gif

# install
target.path = $$[QT_INSTALL_EXAMPLES]/network/multistreamserver
INSTALLS += target
