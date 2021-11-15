LANCELOT_DIR = $$PWD/../../baseline/painting
CONFIG += cmdline moc
TEMPLATE = app
INCLUDEPATH += . $$LANCELOT_DIR
QT += core-private gui-private widgets printsupport openglwidgets

HEADERS += widgets.h \
           interactivewidget.h \
           $$LANCELOT_DIR/paintcommands.h
SOURCES += interactivewidget.cpp \
           main.cpp \
           $$LANCELOT_DIR/paintcommands.cpp
RESOURCES += icons.qrc \
           $$LANCELOT_DIR/images.qrc

qtHaveModule(opengl): QT += opengl

