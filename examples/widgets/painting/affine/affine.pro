SOURCES += main.cpp xform.cpp arthurwidgets.cpp hoverpoints.cpp
HEADERS += xform.h arthurwidgets.h hoverpoints.h

QT += widgets

RESOURCES += affine.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/painting/affine
INSTALLS += target
