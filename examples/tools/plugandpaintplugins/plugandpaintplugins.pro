TEMPLATE      = subdirs
SUBDIRS       = basictools \
                extrafilters

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/plugandpaintplugins
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS plugandpaintplugins.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/plugandpaintplugins
INSTALLS += target sources

symbian: CONFIG += qt_example
QT += widgets
maemo5: CONFIG += qt_example
