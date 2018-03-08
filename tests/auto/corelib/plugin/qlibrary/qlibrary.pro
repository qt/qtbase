QT = core
TEMPLATE    =	subdirs

tst.depends = lib lib2
# lib2 has to be installed after lib, so that plain libmylib.so symlink points
# to version 2 as expected by the test
lib2.depends = lib

SUBDIRS =   lib \
            lib2 \
            tst
TARGET = tst_qlibrary

# no special install rule for subdir
INSTALLS =
