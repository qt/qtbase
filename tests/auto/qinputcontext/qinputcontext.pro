load(qttest_p4)
QT += widgets
SOURCES  += tst_qinputcontext.cpp

symbian {
    LIBS += -lws32 -lcone
}
