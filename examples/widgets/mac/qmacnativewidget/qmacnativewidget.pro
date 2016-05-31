TEMPLATE = app

OBJECTIVE_SOURCES += main.mm
LIBS += -framework Cocoa

QT += widgets
#QT += widgets-private gui-private core-private

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/mac/qmacnativewidget
INSTALLS += target
