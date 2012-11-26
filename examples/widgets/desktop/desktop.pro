TEMPLATE      = subdirs
CONFIG       += ordered
SUBDIRS       = screenshot systray

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/desktop
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS desktop.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/desktop
INSTALLS += target sources

QT += widgets
