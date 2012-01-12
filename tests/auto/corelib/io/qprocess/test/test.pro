CONFIG += testcase
QT = core testlib network
embedded: QT += gui
SOURCES = ../tst_qprocess.cpp

TARGET = ../tst_qprocess

win32:TESTDATA += ../testBatFiles/*

include(../qprocess.pri)
load(testcase) # for target.path and installTestHelperApp()
for(file, SUBPROGRAMS): installTestHelperApp("../$${file}/$${file}",$${file},$${file})
installTestHelperApp("../testProcessSpacesArgs/nospace",testProcessSpacesArgs,nospace)
installTestHelperApp("../testProcessSpacesArgs/one space",testProcessSpacesArgs,"one space")
installTestHelperApp("../testProcessSpacesArgs/two space s",testProcessSpacesArgs,"two space s")
installTestHelperApp("../test Space In Name/testSpaceInName","test Space In Name",testSpaceInName)
