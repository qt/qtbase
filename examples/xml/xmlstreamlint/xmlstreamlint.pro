CONFIG   += console
CONFIG   -= app_bundle
QT       -= gui
SOURCES  += main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/xml/xmlstreamlint
INSTALLS += target
