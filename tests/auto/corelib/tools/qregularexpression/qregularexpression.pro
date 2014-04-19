TEMPLATE = subdirs
SUBDIRS = defaultoptimize forceoptimize
contains(QT_CONFIG,private_tests):SUBDIRS += alwaysoptimize
