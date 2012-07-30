TEMPLATE = subdirs
SUBDIRS = \
        blendbench \
        qimageconversion \
        qimagereader \
        qpixmap \
        qpixmapcache

isEmpty(QT.widgets.name): SUBDIRS -= \
        qimagereader
