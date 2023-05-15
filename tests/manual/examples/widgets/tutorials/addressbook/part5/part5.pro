QT += widgets

SOURCES   = addressbook.cpp \
            finddialog.cpp \
            main.cpp
HEADERS   = addressbook.h \
            finddialog.h

QMAKE_PROJECT_NAME = ab_part5

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tutorials/addressbook/part5
INSTALLS += target
