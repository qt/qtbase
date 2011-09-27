load(qttest_p4)
SOURCES  += tst_qsqlquerymodel.cpp

QT += widgets sql

wince*: {
   DEPLOYMENT_PLUGIN += qsqlite
	LIBS += -lws2
} else {
   win32:LIBS += -lws2_32
}

