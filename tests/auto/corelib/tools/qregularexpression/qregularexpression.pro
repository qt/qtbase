TEMPLATE = subdirs
SUBDIRS = defaultoptimize forceoptimize
qtConfig(private_tests): SUBDIRS += alwaysoptimize
