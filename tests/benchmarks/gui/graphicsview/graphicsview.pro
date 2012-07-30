TEMPLATE = subdirs
SUBDIRS = \
        functional \
        qgraphicsanchorlayout \
        qgraphicsitem \
        #qgraphicslayout \  # FIXME: broken
        qgraphicsscene \
        qgraphicsview \
        qgraphicswidget

isEmpty(QT.widgets.name): SUBDIRS -= \
    qgraphicsanchorlayout \
    qgraphicsitem \
    qgraphicsscene \
    qgraphicsview \
    qgraphicswidget
