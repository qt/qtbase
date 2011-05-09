TEMPLATE = subdirs

SUBDIRS	*= sqldrivers bearer
unix:!symbian {
        contains(QT_CONFIG,iconv)|contains(QT_CONFIG,gnu-libiconv)|contains(QT_CONFIG,sun-libiconv):SUBDIRS *= codecs
} else {
        SUBDIRS *= codecs
}
!contains(QT_CONFIG, no-gui) {
    SUBDIRS *= imageformats
    !embedded:!qpa:SUBDIRS *= graphicssystems
    !win32:!embedded:!mac:!symbian:SUBDIRS *= inputmethods
    !symbian:SUBDIRS += accessible
}
embedded:SUBDIRS *=  gfxdrivers decorations mousedrivers kbddrivers
symbian:SUBDIRS += s60
qpa:SUBDIRS += platforms
