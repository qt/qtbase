QT += printsupport
TEMPLATE = subdirs

osx:   SUBDIRS += cocoa
win32: SUBDIRS += windows
unix:!darwin:qtConfig(cups): SUBDIRS += cups
