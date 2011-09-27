load(qttest_p4)
QT -= gui
SOURCES  += ../tst_qnetworkreply.cpp
TARGET = ../tst_qnetworkreply

qpa:contains(QT_CONFIG,xcb): CONFIG+=insignificant_test  # unstable, QTBUG-21102

win32 {
  CONFIG(debug, debug|release) {
    TARGET = ../../debug/tst_qnetworkreply
} else {
    TARGET = ../../release/tst_qnetworkreply
  }
}

DEFINES += SRCDIR=\\\"$$PWD/..\\\"

QT = core-private network-private
RESOURCES += ../qnetworkreply.qrc

wince* {
    # For cross compiled targets, reference data files need to be deployed
    addFiles.files = ../empty ../rfc3252.txt ../resource ../bigfile ../*.jpg
    addFiles.path = .
    DEPLOYMENT += addFiles

    certFiles.files = ../certs
    certFiles.path    = .
    DEPLOYMENT += certFiles
}
