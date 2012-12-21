TEMPLATE = subdirs
SUBDIRS = \
        functional \
        qgraphicsanchorlayout \
        qgraphicsitem \
        #qgraphicslayout \  # FIXME: broken
        qgraphicsscene \
        qgraphicsview \
        qgraphicswidget

!qtHaveModule(widgets): SUBDIRS -= \
    qgraphicsanchorlayout \
    qgraphicsitem \
    qgraphicsscene \
    qgraphicsview \
    qgraphicswidget
