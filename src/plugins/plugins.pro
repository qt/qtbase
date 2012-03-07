TEMPLATE = subdirs

SUBDIRS *= sqldrivers bearer
!contains(QT_CONFIG, no-gui): SUBDIRS *= imageformats
!contains(QT_CONFIG, no-widgets): SUBDIRS += accessible

SUBDIRS += platforms platforminputcontexts generic

!wince*:SUBDIRS += printsupport
