TEMPLATE = app
TARGET = flightinfo
SOURCES = flightinfo.cpp
FORMS += form.ui
RESOURCES = flightinfo.qrc
QT += network widgets

target.path = $$[QT_INSTALL_EXAMPLES]/embedded/flightinfo
INSTALLS += target
