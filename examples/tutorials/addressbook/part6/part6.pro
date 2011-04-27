SOURCES   = addressbook.cpp \
            finddialog.cpp \
            main.cpp
HEADERS   = addressbook.h \
            finddialog.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tutorials/addressbook/part6
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS part6.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tutorials/addressbook/part6
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
