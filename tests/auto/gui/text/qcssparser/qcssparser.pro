load(qttest_p4)
SOURCES += tst_qcssparser.cpp
QT += xml gui-private

requires(contains(QT_CONFIG,private_tests))
!symbian: {
   DEFINES += SRCDIR=\\\"$$PWD\\\"
}

wince*|symbian: {
   addFiles.files = testdata
   addFiles.path = .
   timesFont.files = C:/Windows/Fonts/times.ttf
   timesFont.path = .
   DEPLOYMENT += addFiles timesFont
}

