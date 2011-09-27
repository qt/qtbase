load(qttest_p4)
SOURCES += tst_qimagereader.cpp
MOC_DIR=tmp
QT += widgets widgets-private core-private gui-private network
RESOURCES += qimagereader.qrc
DEFINES += SRCDIR=\\\"$$PWD\\\"

win32-msvc:QMAKE_CXXFLAGS -= -Zm200
win32-msvc:QMAKE_CXXFLAGS += -Zm800
win32-msvc.net:QMAKE_CXXFLAGS -= -Zm300
win32-msvc.net:QMAKE_CXXFLAGS += -Zm1100

wince*: {
    images.files = images
    images.path = .

    imagePlugins.files = $$QT_BUILD_TREE/plugins/imageformats/*.dll
    imagePlugins.path = imageformats

    DEPLOYMENT += images imagePlugins
    DEFINES += SRCDIR=\\\".\\\"
}
