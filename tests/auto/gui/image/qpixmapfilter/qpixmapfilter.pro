load(qttest_p4)

QT += widgets widgets-private
QT += gui-private

SOURCES  += tst_qpixmapfilter.cpp

wince*: {
    addFiles.files = noise.png
    addFiles.path = .
    DEPLOYMENT += addFiles
}

