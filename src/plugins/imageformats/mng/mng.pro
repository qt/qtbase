TARGET  = qmng
include(../../qpluginbase.pri)

QTDIR_build:REQUIRES = "!contains(QT_CONFIG, no-mng)"

symbian: {
        #Disable warnings in 3rdparty code due to unused variables and arguments
        QMAKE_CXXFLAGS.CW += -W nounused
        TARGET.UID3=0x2001E619
}

include(../../../gui/image/qmnghandler.pri)
SOURCES += main.cpp

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/imageformats
target.path += $$[QT_INSTALL_PLUGINS]/imageformats
INSTALLS += target
