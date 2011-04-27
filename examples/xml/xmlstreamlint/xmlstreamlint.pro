CONFIG   += console
QT       -= gui
QT       += xml
SOURCES  += main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/xml/xmlstreamlint
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS xmlstreamlint.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/xml/xmlstreamlint
INSTALLS += target sources

symbian: CONFIG += qt_example
