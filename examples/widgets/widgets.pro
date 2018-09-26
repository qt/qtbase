requires(qtHaveModule(widgets))

TEMPLATE      = subdirs
CONFIG += no_docs_target

SUBDIRS       = \
                animation \
                desktop \
                dialogs \
                draganddrop \
                effects \
                gestures \
                graphicsview \
                itemviews \
                layouts \
                mainwindows \
                painting \
                richtext \
                scroller \
                statemachine \
                tools \
                tutorials \
                widgets

qtHaveModule(gui):qtConfig(opengl): \
    SUBDIRS += windowcontainer

contains(DEFINES, QT_NO_CURSOR): SUBDIRS -= mainwindows
!qtConfig(draganddrop): SUBDIRS -= draganddrop
!qtConfig(animation): SUBDIRS -= animation
mac:SUBDIRS += mac
