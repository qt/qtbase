TEMPLATE      = subdirs
CONFIG += no_docs_target

SUBDIRS       = \
                animation \
                desktop \
                dialogs \
                draganddrop \
                effects \
                graphicsview \
                itemviews \
                layouts \
                mainwindows \
                painting \
                richtext \
                scroller \
                statemachine \
                tutorials \
                widgets

contains(DEFINES, QT_NO_CURSOR): SUBDIRS -= mainwindows

# install
sources.files = *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets
INSTALLS += sources