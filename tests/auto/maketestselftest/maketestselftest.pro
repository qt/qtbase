TEMPLATE = subdirs
SUBDIRS = checktest test
test.depends = checktest

requires(!cross_compile)

