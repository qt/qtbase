TEMPLATE = subdirs
SUBDIRS = \
        blendbench \
        qimageconversion \
        qimagereader \
        qimagescale \
        qpixmap \
        qpixmapcache

!qtHaveModule(widgets)|!qtHaveModule(network): SUBDIRS -= \
        qimagereader
