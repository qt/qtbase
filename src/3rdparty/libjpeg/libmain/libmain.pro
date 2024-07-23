TARGET = qtlibjpeg

CONFIG += installed

MODULE_INCLUDEPATH = $$PWD/../src
MODULE_EXT_HEADERS = $$PWD/../src/jpeglib.h \
                     $$PWD/../src/jerror.h \
                     $$PWD/../src/jconfig.h \
                     $$PWD/../src/jmorecfg.h

include($$PWD/../common.pri)

SOURCES = $$JPEG_SOURCES

objdir = $$OBJECTS_DIR
android {
    objdir = $$objdir/$${QT_ARCH}
}
windows|qtConfig(debug_and_release) {
    CONFIG(debug, debug|release): objdir = $$objdir/debug
    else: objdir = $$objdir/release
}

for(srcfile, JPEG12_SOURCES) {
    objfile = $$basename(srcfile)
    objfile = $$replace(objfile, \.c, $${QMAKE_EXT_OBJ})
    OBJECTS += $${OUT_PWD}/../lib12bits/$${objdir}/$$objfile
}

for(srcfile, JPEG16_SOURCES) {
    objfile = $$basename(srcfile)
    objfile = $$replace(objfile, \.c, $${QMAKE_EXT_OBJ})
    OBJECTS += $${OUT_PWD}/../lib16bits/$${objdir}/$$objfile
}
