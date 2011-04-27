HEADERS       = chatdialog.h \
                client.h \
                connection.h \
                peermanager.h \
                server.h
SOURCES       = chatdialog.cpp \
                client.cpp \
                connection.cpp \
                main.cpp \
                peermanager.cpp \
                server.cpp
FORMS         = chatdialog.ui
QT           += network

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/network/network-chat
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS network-chat.pro *.chat
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/network/network-chat
INSTALLS += target sources

symbian {
    include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
    LIBS += -lcharconv
    TARGET.CAPABILITY = "NetworkServices ReadUserData WriteUserData"
    TARGET.EPOCHEAPSIZE = 0x20000 0x2000000
}
