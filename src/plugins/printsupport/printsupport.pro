TEMPLATE = subdirs
QT_FOR_CONFIG += printsupport-private

unix:!darwin:qtConfig(cups): SUBDIRS += cups
