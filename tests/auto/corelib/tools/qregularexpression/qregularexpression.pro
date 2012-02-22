TEMPLATE = subdirs
SUBDIRS = defaultoptimize
contains(QT_CONFIG,private_tests):SUBDIRS += alwaysoptimize
