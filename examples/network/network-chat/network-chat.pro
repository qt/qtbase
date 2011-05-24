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
    CONFIG += qt_example
    LIBS += -lcharconv
    TARGET.CAPABILITY = "NetworkServices ReadUserData WriteUserData"
    TARGET.EPOCHEAPSIZE = 0x20000 0x2000000
}
maemo5: include($$QT_SOURCE_TREE/examples/maemo5pkgrules.pri)

symbian: warning(This example might not fully work on Symbian platform)
maemo5: warning(This example might not fully work on Maemo platform)
