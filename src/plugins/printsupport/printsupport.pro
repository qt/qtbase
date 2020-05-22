TEMPLATE = subdirs
QT_FOR_CONFIG += printsupport-private

win32: SUBDIRS += windows
unix:!darwin:qtConfig(cups): SUBDIRS += cups
