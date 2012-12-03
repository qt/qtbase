CONFIG += testcase
mac:CONFIG -= app_bundle
# CONFIG += parallel_test // Cannot be parallel due to the activation test
TARGET = tst_qwindowcontainer
QT += widgets testlib
SOURCES += tst_qwindowcontainer.cpp
