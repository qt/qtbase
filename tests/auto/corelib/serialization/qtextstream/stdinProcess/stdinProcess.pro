SOURCES += main.cpp
QT = core
CONFIG += cmdline
DESTDIR = ./

# This app is testdata for tst_qtextstream
target.path = $$[QT_INSTALL_TESTS]/tst_qtextstream/$$TARGET
INSTALLS += target
