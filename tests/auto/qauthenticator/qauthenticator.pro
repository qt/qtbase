load(qttest_p4)
requires(contains(QT_CONFIG,private_tests))
QT = core network
SOURCES  += tst_qauthenticator.cpp
DEFINES += SRCDIR=\\\"$$PWD/\\\"
