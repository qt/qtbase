TEMPLATE      = subdirs
SUBDIRS       = windows

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/qpa
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS qpa.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/qpa
INSTALLS += target sources
