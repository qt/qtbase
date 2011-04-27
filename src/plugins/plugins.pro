TEMPLATE = subdirs

SUBDIRS	*= sqldrivers script bearer
unix:!symbian {
        contains(QT_CONFIG,iconv)|contains(QT_CONFIG,gnu-libiconv)|contains(QT_CONFIG,sun-libiconv):SUBDIRS *= codecs
} else {
        SUBDIRS *= codecs
}
!contains(QT_CONFIG, no-gui): SUBDIRS *= imageformats iconengines
!embedded:!qpa:SUBDIRS *= graphicssystems
embedded:SUBDIRS *=  gfxdrivers decorations mousedrivers kbddrivers
!win32:!embedded:!mac:!symbian:SUBDIRS *= inputmethods
!symbian:!contains(QT_CONFIG, no-gui):SUBDIRS += accessible
symbian:SUBDIRS += s60
contains(QT_CONFIG, phonon): SUBDIRS *= phonon
qpa:SUBDIRS += platforms
contains(QT_CONFIG, declarative): SUBDIRS *= qmltooling
