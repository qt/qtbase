QT = core
TEMPLATE    =	subdirs

tst.depends = lib lib2

SUBDIRS =   lib \
            lib2 \
            tst
TARGET = tst_qlibrary

# no special install rule for subdir
INSTALLS =
