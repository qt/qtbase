TEMPLATE = app

# needs gui-private to be able to include <rhi/qrhi.h>
QT += gui-private widgets

HEADERS += examplewidget.h
SOURCES += examplewidget.cpp main.cpp

RESOURCES += cuberhiwidget.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/widgets/rhi/cuberhiwidget
INSTALLS += target
