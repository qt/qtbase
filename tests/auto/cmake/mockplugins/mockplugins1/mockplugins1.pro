TARGET = QtMockPlugins1
QT = core
MODULE_PLUGIN_TYPES = mockplugin

# Fake a git_build, to force qmake to run syncqt.pl when doing a standalone tests build
# like it is done in Coin, otherwise module headers would not be generated.
CONFIG += git_build
HEADERS += qmockplugin.h
SOURCES += fake.cpp # Needed to make libtool / ar happy on macOS
load(qt_module)
