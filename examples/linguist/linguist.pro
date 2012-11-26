TEMPLATE      = subdirs
SUBDIRS       = arrowpad \
                hellotr \
                trollprint

# install
sources.files = README *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/linguist
INSTALLS += sources

QT += widgets
