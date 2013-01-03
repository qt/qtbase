TEMPLATE = subdirs
SUBDIRS = \
        blendbench \
        qimageconversion \
        qimagereader \
        qpixmap \
        qpixmapcache

!qtHaveModule(widgets): SUBDIRS -= \
        qimagereader
