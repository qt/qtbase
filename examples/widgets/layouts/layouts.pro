TEMPLATE      = subdirs
SUBDIRS       = basiclayouts \
                borderlayout \
                dynamiclayouts \
                flowlayout

# install
sources.files = README *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/layouts
INSTALLS += sources

QT += widgets
