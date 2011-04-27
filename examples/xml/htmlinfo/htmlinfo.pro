SOURCES += main.cpp
QT -= gui

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
    include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
}
