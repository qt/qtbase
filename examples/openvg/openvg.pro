TEMPLATE      = subdirs
SUBDIRS       = star

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/openvg
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS openvg.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/openvg
INSTALLS += target sources

