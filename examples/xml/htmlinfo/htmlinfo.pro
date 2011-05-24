SOURCES += main.cpp
QT -= gui

RESOURCES   = resources.qrc

win32: CONFIG += console

wince*|symbian:{
   htmlfiles.files = *.html
   htmlfiles.path = .
   DEPLOYMENT += htmlfiles
}

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/xml/htmlinfo
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.html htmlinfo.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/xml/htmlinfo
INSTALLS += target sources

symbian {
    TARGET.UID3 = 0xA000C609
    CONFIG += qt_example
}
maemo5: CONFIG += qt_example

symbian: warning(This example does not work on Symbian platform)
