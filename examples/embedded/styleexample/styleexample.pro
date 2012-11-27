QT += widgets

HEADERS += stylewidget.h
FORMS += stylewidget.ui
SOURCES += main.cpp stylewidget.cpp
RESOURCES += styleexample.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/embedded/styleexample
INSTALLS += target
