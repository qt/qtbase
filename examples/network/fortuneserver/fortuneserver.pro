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
    include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
    TARGET.CAPABILITY = "NetworkServices ReadUserData"
    TARGET.EPOCHEAPSIZE = 0x20000 0x2000000
}
