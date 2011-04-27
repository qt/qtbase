TEMPLATE = subdirs

!contains(QT_CONFIG, no-jpeg):!contains(QT_CONFIG, jpeg):SUBDIRS += jpeg
!contains(QT_CONFIG, no-gif):!contains(QT_CONFIG, gif):SUBDIRS += gif
!contains(QT_CONFIG, no-mng):!contains(QT_CONFIG, mng):SUBDIRS += mng
contains(QT_CONFIG, svg):SUBDIRS += svg
!contains(QT_CONFIG, no-tiff):!contains(QT_CONFIG, tiff):SUBDIRS += tiff
!contains(QT_CONFIG, no-ico):SUBDIRS += ico
