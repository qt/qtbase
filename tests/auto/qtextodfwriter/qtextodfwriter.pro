load(qttest_p4)
SOURCES += tst_qtextodfwriter.cpp

!symbian:DEFINES += SRCDIR=\\\"$$PWD\\\"
symbian:INCLUDEPATH+=$$[QT_INSTALL_PREFIX]/include/QtGui/private
