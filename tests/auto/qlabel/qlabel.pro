load(qttest_p4)

QT += widgets widgets-private
QT += core-private gui-private

SOURCES += tst_qlabel.cpp
wince*::DEFINES += SRCDIR=\\\"\\\"
else:!symbian:DEFINES += SRCDIR=\\\"$$PWD/\\\"
wince*|symbian { 
    addFiles.files = *.png \
        testdata
    addFiles.path = .
    DEPLOYMENT += addFiles
}

CONFIG += insignificant_test # QTBUG-21402
