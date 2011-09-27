load(qttest_p4)
SOURCES  += tst_qimage.cpp

QT += core-private gui-private

wince*: {
   addImages.files = images/*
   addImages.path = images
   DEPLOYMENT += addImages
   DEFINES += SRCDIR=\\\".\\\"
} else {
   DEFINES += SRCDIR=\\\"$$PWD\\\"
}
