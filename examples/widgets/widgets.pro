requires(qtHaveModule(widgets))

TEMPLATE      = subdirs
CONFIG += no_docs_target

SUBDIRS       = \
                animation \
                desktop \
                dialogs \
                draganddrop \
                gallery \
                gestures \
                graphicsview \
                itemviews \
                layouts \
                mainwindows \
                painting \
                richtext \
                tools \
                touch \
                tutorials \
                widgets

contains(DEFINES, QT_NO_CURSOR): SUBDIRS -= mainwindows
!qtConfig(draganddrop): SUBDIRS -= draganddrop
!qtConfig(animation): SUBDIRS -= animation
