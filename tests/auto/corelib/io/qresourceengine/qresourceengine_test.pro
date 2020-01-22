CONFIG += testcase
TARGET = tst_qresourceengine

QT = core-private testlib
SOURCES = tst_qresourceengine.cpp
RESOURCES += testqrc/test.qrc

qtPrepareTool(QMAKE_RCC, rcc, _DEP)
runtime_resource.target = runtime_resource.rcc
runtime_resource.depends = $$PWD/testqrc/test.qrc $$QMAKE_RCC_EXE
runtime_resource.commands = $$QMAKE_RCC -root /runtime_resource/ -binary $$PWD/testqrc/test.qrc -o $${runtime_resource.target}
QMAKE_EXTRA_TARGETS = runtime_resource
PRE_TARGETDEPS += $${runtime_resource.target}
QMAKE_DISTCLEAN += $${runtime_resource.target}

TESTDATA += \
    parentdir.txt \
    testqrc/* \
    *.rcc
GENERATED_TESTDATA = $${runtime_resource.target}

android:!android-embedded {
    RESOURCES += android_testdata.qrc
}

win32:debug_and_release {
    CONFIG(debug, debug|release): LIBS += -Lstaticplugin/debug
    else: LIBS += -Lstaticplugin/release
} else {
    LIBS += -Lstaticplugin
}
LIBS += -lmoctestplugin

builtin_testdata: DEFINES += BUILTIN_TESTDATA
