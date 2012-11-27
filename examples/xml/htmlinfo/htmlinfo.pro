SOURCES += main.cpp
QT -= gui
CONFIG -= app_bundle

RESOURCES   = resources.qrc

win32: CONFIG += console

wince*:{
   htmlfiles.files = *.html
   htmlfiles.path = .
   DEPLOYMENT += htmlfiles
}

# install
target.path = $$[QT_INSTALL_EXAMPLES]/xml/htmlinfo
INSTALLS += target
