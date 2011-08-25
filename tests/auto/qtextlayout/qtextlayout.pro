load(qttest_p4)
QT += core-private gui-private
HEADERS += 
SOURCES += tst_qtextlayout.cpp 
DEFINES += QT_COMPILES_IN_HARFBUZZ
INCLUDEPATH += $$QT_SOURCE_TREE/src/3rdparty/harfbuzz/src

symbian {
	TARGET.EPOCHEAPSIZE = 100000 20000000
}

qpa:contains(QT_CONFIG,qpa):CONFIG+=insignificant_test  # QTBUG-20979
