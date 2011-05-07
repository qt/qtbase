SOURCES = addressbook.cpp \
          main.cpp
HEADERS = addressbook.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tutorials/addressbook/part4
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS part4.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tutorials/addressbook/part4
INSTALLS += target sources
QT += widgets
