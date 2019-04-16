TEMPLATE = subdirs
QT_FOR_CONFIG += gui-private

qtConfig(xkbcommon): SUBDIRS += xkbcommon

SUBDIRS += input-support.pro ### FIXME - QTBUG-52657

CONFIG += ordered
