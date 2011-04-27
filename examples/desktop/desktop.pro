TEMPLATE      = subdirs
CONFIG       += ordered
SUBDIRS       = screenshot

!symbian:contains(QT_CONFIG, svg): SUBDIRS += systray

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/desktop
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS desktop.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/desktop
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
