SOURCES += main.cpp composition.cpp
HEADERS += composition.h

SHARED_FOLDER = ../shared

include($$SHARED_FOLDER/shared.pri)

RESOURCES += composition.qrc
QT += widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/painting/composition
INSTALLS += target
