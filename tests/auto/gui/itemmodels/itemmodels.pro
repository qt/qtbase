TEMPLATE=subdirs
SUBDIRS= \
    qstandarditem \
    qstandarditemmodel

isEmpty(QT.widgets.name):SUBDIRS -= \
    qstandarditemmodel
