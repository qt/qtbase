QT += widgets

SOURCES   = addressbook.cpp \
            finddialog.cpp \
            main.cpp
HEADERS   = addressbook.h \
            finddialog.h

QMAKE_PROJECT_NAME = abfr_part7

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tutorials/addressbook-fr/part7
INSTALLS += target

simulator: warning(This example might not fully work on Simulator platform)
