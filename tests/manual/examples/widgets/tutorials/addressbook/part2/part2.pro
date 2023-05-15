QT += widgets

SOURCES   = addressbook.cpp \
            main.cpp
HEADERS   = addressbook.h

QMAKE_PROJECT_NAME = ab_part2

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tutorials/addressbook/part2
INSTALLS += target
