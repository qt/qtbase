TEMPLATE = subdirs

SUBDIRS *= sqldrivers bearer
!contains(QT_CONFIG, no-gui): SUBDIRS *= imageformats platforms platforminputcontexts generic
!contains(QT_CONFIG, no-widgets): SUBDIRS += accessible

!wince*:!contains(QT_CONFIG, no-widgets):SUBDIRS += printsupport
