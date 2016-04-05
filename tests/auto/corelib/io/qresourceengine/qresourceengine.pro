CONFIG += testcase
TARGET = tst_qresourceengine
load(resources)
QT = core testlib
SOURCES = tst_qresourceengine.cpp
RESOURCES += testqrc/test.qrc

runtime_resource.target = runtime_resource.rcc
runtime_resource.depends = $$PWD/testqrc/test.qrc
runtime_resource.commands = $$QMAKE_RCC -root /runtime_resource/ -binary $${runtime_resource.depends} -o $${runtime_resource.target}
QMAKE_EXTRA_TARGETS = runtime_resource
PRE_TARGETDEPS += $${runtime_resource.target}
QMAKE_DISTCLEAN += $${runtime_resource.target}

TESTDATA += \
    parentdir.txt \
    testqrc/*
GENERATED_TESTDATA = $${runtime_resource.target}

android {
    RESOURCES += android_testdata.qrc
}
