HEADERS       = dialog.h
SOURCES       = dialog.cpp \
                main.cpp
QT           += network

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/network/loopback
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS loopback.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/network/loopback
INSTALLS += target sources

symbian: {
    CONFIG += qt_example
    TARGET.CAPABILITY = NetworkServices
}
maemo5: CONFIG += qt_example
