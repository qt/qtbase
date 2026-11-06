# Check that moc resolves a bare include through the framework forwarders
# when a header-only module adds the naked Qt include dir, as QtQmlIntegration
# does, between the forwarding include dir and the framework path.

TEMPLATE = lib
CONFIG += plugin

QT.fakeheaderonly.name = QtFakeHeaderOnly
QT.fakeheaderonly.module =
QT.fakeheaderonly.includes = $$[QT_INSTALL_HEADERS]
QT.fakeheaderonly.depends = core
QT.fakeheaderonly.CONFIG = no_link

QT += gui fakeheaderonly

HEADERS += plugin.h
