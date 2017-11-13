TEMPLATE = app
TARGET = notepad

QT += printsupport

SOURCES += \
    main.cpp\
    notepad.cpp

HEADERS += notepad.h

FORMS += notepad.ui

RESOURCES += \
    notepad.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/tutorials/notepad
INSTALLS += target

