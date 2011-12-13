QT = core testlib
SOURCES += tst_xunit.cpp


mac:CONFIG -= app_bundle
CONFIG -= debug_and_release_target

TARGET = xunit

# This app is testdata for tst_selftests
target.path = $$[QT_INSTALL_TESTS]/tst_selftests/$$TARGET
INSTALLS += target

