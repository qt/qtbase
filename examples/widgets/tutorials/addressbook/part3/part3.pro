QT += widgets

SOURCES = addressbook.cpp \
          main.cpp
HEADERS = addressbook.h

QMAKE_PROJECT_NAME = ab_part3

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tutorials/addressbook/part3
INSTALLS += target

simulator: warning(This example might not fully work on Simulator platform)
