TEMPLATE = subdirs

SUBDIRS *= sqldrivers bearer
!contains(QT_CONFIG, no-gui): SUBDIRS *= imageformats
!isEmpty(QT.widgets.name):    SUBDIRS += accessible

SUBDIRS += platforms platforminputcontexts printsupport generic
