CONFIG   += console
QT       -= gui
QT       += xml widgets
SOURCES  += main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/xml/xmlstreamlint
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS xmlstreamlint.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/xml/xmlstreamlint
INSTALLS += target sources


simulator: warning(This example does not work on Simulator platform)
