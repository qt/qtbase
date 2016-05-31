TEMPLATE = app

OBJECTIVE_SOURCES += main.mm
LIBS += -framework Cocoa

QT += widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/mac/qmaccocoaviewcontainer
INSTALLS += target
