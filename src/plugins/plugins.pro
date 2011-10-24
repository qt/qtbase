TEMPLATE = subdirs

SUBDIRS	*= sqldrivers bearer
unix:!symbian {
        contains(QT_CONFIG,iconv)|contains(QT_CONFIG,gnu-libiconv)|contains(QT_CONFIG,sun-libiconv):SUBDIRS *= codecs
} else {
        SUBDIRS *= codecs
}
!contains(QT_CONFIG, no-gui): SUBDIRS *= imageformats
!symbian:!contains(QT_CONFIG, no-gui):SUBDIRS += accessible

symbian:SUBDIRS += s60
qpa: {
    SUBDIRS += platforms
    SUBDIRS += platforminputcontexts
    SUBDIRS += printsupport
}
