QT += widgets

SOURCES = addressbook.cpp \
          main.cpp
HEADERS = addressbook.h

QMAKE_PROJECT_NAME = ab_part4

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tutorials/addressbook/part4
INSTALLS += target
