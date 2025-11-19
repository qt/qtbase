TEMPLATE = subdirs
QT_FOR_CONFIG += gui-private

qtConfig(xkbcommon): SUBDIRS += xkbcommon

### FIXME - QTBUG-52657
SUBDIRS += input-support.pro

CONFIG += ordered
