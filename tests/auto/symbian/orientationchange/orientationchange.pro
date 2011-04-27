load(qttest_p4)
HEADERS +=
SOURCES += tst_orientationchange.cpp

symbian {
    INCLUDEPATH += $$MW_LAYER_SYSTEMINCLUDE
    LIBS += -lcone -leikcore -lavkon # Screen orientation
}
