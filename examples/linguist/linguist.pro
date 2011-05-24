TEMPLATE      = subdirs
SUBDIRS       = arrowpad \
                hellotr \
                trollprint

# install
sources.files = README *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/linguist
INSTALLS += sources

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example
