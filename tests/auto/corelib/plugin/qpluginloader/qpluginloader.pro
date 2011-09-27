QT = core
TEMPLATE    =	subdirs
CONFIG  += ordered
SUBDIRS	=	lib \
                theplugin \
		tst
!win32: !macx-*: SUBDIRS += almostplugin
TARGET = tst_qpluginloader

# no special install rule for subdir
INSTALLS =


CONFIG += parallel_test
