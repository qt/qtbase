HEADERS       = client.h
SOURCES       = client.cpp \
                main.cpp
QT           += network

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/network/fortuneclient
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS fortuneclient.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/network/fortuneclient
INSTALLS += target sources

symbian {
    include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
    TARGET.CAPABILITY = "NetworkServices ReadUserData WriteUserData"
    TARGET.EPOCHEAPSIZE = 0x20000 0x2000000
}
