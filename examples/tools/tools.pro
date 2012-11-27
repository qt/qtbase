TEMPLATE      = subdirs
CONFIG       += ordered
SUBDIRS       = contiguouscache \
                customtype \
                customtypesending

# install
target.path = $$[QT_INSTALL_EXAMPLES]/tools
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS tools.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/tools
INSTALLS += target sources
