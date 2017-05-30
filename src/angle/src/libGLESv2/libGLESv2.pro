include(../common/gles_common.pri)

DEF_FILE_TARGET = $${TARGET}

TARGET = $$qtLibraryTarget($${LIBGLESV2_NAME})

!static {
    DEF_FILE = $$ANGLE_DIR/src/libGLESv2/$${DEF_FILE_TARGET}.def
    mingw: equals(QT_ARCH, i386): DEF_FILE = $$ANGLE_DIR/src/libGLESv2/$${DEF_FILE_TARGET}_mingw32.def
} else {
    DEFINES += DllMain=DllMain_ANGLE # prevent symbol from conflicting with the user's DllMain
}
