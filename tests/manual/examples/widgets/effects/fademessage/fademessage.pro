QT += widgets

SOURCES += main.cpp fademessage.cpp
HEADERS += fademessage.h
RESOURCES += fademessage.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/effects/fademessage
INSTALLS += target
