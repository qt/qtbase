QT += widgets

SOURCES   = addressbook.cpp \
            main.cpp
HEADERS   = addressbook.h

QMAKE_PROJECT_NAME = abfr_part2

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tutorials/addressbook-fr/part2
INSTALLS += target

simulator: warning(This example might not fully work on Simulator platform)
