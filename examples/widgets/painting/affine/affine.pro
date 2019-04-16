SOURCES += main.cpp xform.cpp
HEADERS += xform.h

QT += widgets

SHARED_FOLDER = ../shared

include($$SHARED_FOLDER/shared.pri)

RESOURCES += affine.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/painting/affine
INSTALLS += target
