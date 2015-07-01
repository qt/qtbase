CONFIG += testcase
CONFIG += parallel_test
TARGET = tst_qsqlquerymodel
SOURCES  += tst_qsqlquerymodel.cpp

QT += widgets sql testlib core-private sql-private

wince {
   DEPLOYMENT_PLUGIN += qsqlite
	LIBS += -lws2
} else {
   win32:LIBS += -lws2_32
}

