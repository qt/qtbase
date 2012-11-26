QT += widgets

HEADERS       = receiver.h
SOURCES       = receiver.cpp \
                main.cpp
QT           += network

# install
target.path = $$[QT_INSTALL_EXAMPLES]/network/multicastreceiver
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS multicastreceiver.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/network/multicastreceiver
INSTALLS += target sources

