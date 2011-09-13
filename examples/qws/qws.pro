TEMPLATE      = subdirs
# no /dev/fbX
!qnx:!vxworks:SUBDIRS = framebuffer
SUBDIRS      += mousecalibration simpledecoration

# install
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS README *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/qws
INSTALLS += sources
QT += widgets
