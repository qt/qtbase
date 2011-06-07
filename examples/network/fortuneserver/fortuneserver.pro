HEADERS       = server.h
SOURCES       = server.cpp \
                main.cpp
QT           += network

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/network/fortuneserver
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS fortuneserver.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/network/fortuneserver
INSTALLS += target sources

symbian {
    TARGET.UID3 = 0xA000E406
    CONFIG += qt_example
    TARGET.CAPABILITY = "NetworkServices ReadUserData"
    TARGET.EPOCHEAPSIZE = 0x20000 0x2000000
}
maemo5: CONFIG += qt_example

symbian: warning(This example might not fully work on Symbian platform)
maemo5: warning(This example might not fully work on Maemo platform)
simulator: warning(This example might not fully work on Simulator platform)
