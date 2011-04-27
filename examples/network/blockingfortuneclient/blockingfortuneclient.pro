HEADERS       = blockingclient.h \
                fortunethread.h
SOURCES       = blockingclient.cpp \
                main.cpp \
                fortunethread.cpp
QT           += network

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/network/blockingfortuneclient
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS blockingfortuneclient.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/network/blockingfortuneclient
INSTALLS += target sources

symbian: {
    CONFIG += qt_example
    TARGET.CAPABILITY = NetworkServices
}
maemo5: CONFIG += qt_example

symbian: warning(This example might not fully work on Symbian platform)
maemo5: warning(This example might not fully work on Maemo platform)
