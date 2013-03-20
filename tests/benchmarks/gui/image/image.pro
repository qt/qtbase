TEMPLATE = subdirs
SUBDIRS = \
        blendbench \
        qimageconversion \
        qimagereader \
        qpixmap \
        qpixmapcache

!qtHaveModule(widgets)|!qtHaveModule(network): SUBDIRS -= \
        qimagereader
