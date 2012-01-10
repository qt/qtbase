CONFIG += testcase
SOURCES += ../tst_selftests.cpp
QT = core xml testlib-private

TARGET = ../tst_selftests

win32 {
  CONFIG(debug, debug|release) {
    TARGET = ../../debug/tst_selftests
} else {
    TARGET = ../../release/tst_selftests
  }
}

RESOURCES += ../selftests.qrc

include(../selftests.pri)
load(testcase) # for target.path and installTestHelperApp()
for(file, SUBPROGRAMS): installTestHelperApp("../$${file}/$${file}",$${file},$${file})

