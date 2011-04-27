TEMPLATE      = subdirs
SUBDIRS       = arrowpad \
                hellotr \
                trollprint

# install
sources.files = README *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/linguist
INSTALLS += sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
