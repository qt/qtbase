load(qttest_p4)
TARGET = tst_qfileopenevent
HEADERS +=
SOURCES += tst_qfileopenevent.cpp
symbian {
    LIBS+=-lefsrv -lapgrfx -lapmime
}
