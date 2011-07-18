load(qttest_p4)

QT += gui-private

SOURCES += tst_qlayout.cpp
wince*|symbian: {
   addFiles.files = baseline
   addFiles.path = .
   DEPLOYMENT += addFiles
} else {
   DEFINES += SRCDIR=\\\"$$PWD\\\"

   test_data.files = baseline/*
   test_data.path =  $${target.path}/baseline
   INSTALLS += test_data
}

CONFIG+=insignificant_test
