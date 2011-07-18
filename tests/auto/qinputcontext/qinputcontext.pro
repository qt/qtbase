load(qttest_p4)
SOURCES  += tst_qinputcontext.cpp

symbian {
    LIBS += -lws32 -lcone
}

mac*:CONFIG+=insignificant_test
