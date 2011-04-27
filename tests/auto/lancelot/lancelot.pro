load(qttest_p4)
QT += xml svg
contains(QT_CONFIG, opengl)|contains(QT_CONFIG, opengles1)|contains(QT_CONFIG, opengles2):QT += opengl

SOURCES += tst_lancelot.cpp \
           $$PWD/../../arthur/common/paintcommands.cpp
HEADERS += $$PWD/../../arthur/common/paintcommands.h
RESOURCES += $$PWD/../../arthur/common/images.qrc

include($$PWD/../../arthur/common/qbaselinetest.pri)

!symbian:!wince*:DEFINES += SRCDIR=\\\"$$PWD\\\"
linux-g++-maemo:DEFINES += USE_RUNTIME_DIR
