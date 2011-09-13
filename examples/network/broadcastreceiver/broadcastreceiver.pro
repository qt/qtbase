QT += widgets

HEADERS       = receiver.h
SOURCES       = receiver.cpp \
                main.cpp
QT           += network

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/network/broadcastreceiver
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS broadcastreceiver.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/network/broadcastreceiver
INSTALLS += target sources

symbian: {
    CONFIG += qt_example
    TARGET.CAPABILITY = NetworkServices
}
maemo5: CONFIG += qt_example
