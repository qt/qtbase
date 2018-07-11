SOURCES += main.cpp gradients.cpp
HEADERS += gradients.h

SHARED_FOLDER = ../shared

include($$SHARED_FOLDER/shared.pri)

RESOURCES += gradients.qrc
QT += widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/painting/gradients
INSTALLS += target

