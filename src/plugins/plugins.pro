TEMPLATE = subdirs

SUBDIRS	*= sqldrivers bearer
unix {
        contains(QT_CONFIG,iconv)|contains(QT_CONFIG,gnu-libiconv)|contains(QT_CONFIG,sun-libiconv):SUBDIRS *= codecs
} else {
        SUBDIRS *= codecs
}
!contains(QT_CONFIG, no-gui): SUBDIRS *= imageformats
!contains(QT_CONFIG, no-gui):SUBDIRS += accessible

SUBDIRS += platforms platforminputcontexts printsupport
