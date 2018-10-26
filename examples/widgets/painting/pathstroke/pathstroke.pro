SOURCES += main.cpp pathstroke.cpp
HEADERS += pathstroke.h

SHARED_FOLDER = ../shared

include($$SHARED_FOLDER/shared.pri)

RESOURCES += pathstroke.qrc

QT += widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/painting/pathstroke
INSTALLS += target

