CONFIG += qttest_p4

QT = core-private gui

SOURCES		+= tst_qfilesystemmodel.cpp
TARGET		= tst_qfilesystemmodel

symbian: {
    HEADERS += ../../../include/qtgui/private/qfileinfogatherer_p.h

    # need to deploy something to create the private directory
    dummyDeploy.files = tst_qfilesystemmodel.cpp
    dummyDeploy.path = .
    DEPLOYMENT += dummyDeploy
    LIBS += -lefsrv
}
