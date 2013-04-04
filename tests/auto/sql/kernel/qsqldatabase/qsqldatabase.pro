CONFIG += testcase
TARGET = tst_qsqldatabase
SOURCES  += tst_qsqldatabase.cpp

QT = core sql testlib core-private sql-private

win32: {
   !wince*: LIBS += -lws2_32
   else: LIBS += -lws2
}

wince*: {
   DEPLOYMENT_PLUGIN += qsqlite

   testData.files = testdata
   testData.path = .

   DEPLOYMENT += testData
}
