TEMPLATE      = subdirs
CONFIG       += ordered
SUBDIRS       = screenshot

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/desktop
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS desktop.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/desktop
INSTALLS += target sources

symbian: CONFIG += qt_example
QT += widgets
