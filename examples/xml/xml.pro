TEMPLATE      = subdirs
SUBDIRS       = htmlinfo \
                xmlstreamlint

!contains(QT_CONFIG, no-gui) {
    SUBDIRS +=  dombookmarks \
                rsslisting \
                saxbookmarks \
                streambookmarks
}

symbian: SUBDIRS = htmlinfo saxbookmarks

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/xml
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS xml.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/xml
INSTALLS += target sources

symbian: CONFIG += qt_example
QT += widgets
maemo5: CONFIG += qt_example
