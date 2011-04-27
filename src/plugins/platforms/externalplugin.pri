#
# Lighthouse now has preliminarily support for building and
# loading platform plugins from outside the Qt source/build
# tree.
#
# 1) Building external plugins:
#    Set QTDIR to the Qt build directory, copy this file to
#    the plugin source repository and include it at the top
#    of the plugin's pro file. Use QT_SOURCE_TREE if you
#    want to pull in source code from Qt:
#
#    include($$QT_SOURCE_TREE/src/plugins/platforms/fontdatabases/genericunix/genericunix.pri)
#
# 2) Loading external plugins:
#    Specify the path to the directory containing the
#    plugin on the command line, in addition to the
#    platform name.
#
#    ./wiggly -platformPluginPath /path/to/myPlugin -platform gullfaksA
#

!exists($$(QTDIR)/.qmake.cache) {
    error("Please set QTDIR to the Qt build directory")
}

QT_SOURCE_TREE = $$fromfile($$(QTDIR)/.qmake.cache,QT_SOURCE_TREE)
QT_BUILD_TREE = $$fromfile($$(QTDIR)/.qmake.cache,QT_BUILD_TREE)

include($$QT_SOURCE_TREE/src/plugins/qpluginbase.pri)
