CONFIG += testcase parallel_test
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

TESTDATA += \
    parentdir.txt \
    testqrc/*

# Special case needed for runtime_resource.rcc installation,
# since it does not exist at qmake runtime.
load(testcase)  # to get value of target.path
runtime_resource_install.CONFIG = no_check_exist
runtime_resource_install.files = $$OUT_PWD/$${runtime_resource.target}
runtime_resource_install.path = $${target.path}
INSTALLS += runtime_resource_install
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
