SOURCES   = addressbook.cpp \
            main.cpp
HEADERS   = addressbook.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tutorials/addressbook/part1
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS part1.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tutorials/addressbook/part1
INSTALLS += target sources

QT += widgets

simulator: warning(This example might not fully work on Simulator platform)
