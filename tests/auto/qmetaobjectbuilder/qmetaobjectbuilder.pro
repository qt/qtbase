load(qttest_p4)
macx:CONFIG -= app_bundle

SOURCES += \
    tst_qmetaobjectbuilder.cpp

CONFIG += parallel_test
QT += core-private gui-private
