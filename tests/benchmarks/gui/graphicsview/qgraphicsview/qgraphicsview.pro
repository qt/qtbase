load(qttest_p4)
TEMPLATE = app
TARGET = tst_bench_qgraphicsview

SOURCES += tst_qgraphicsview.cpp
RESOURCES += qgraphicsview.qrc

include(chiptester/chiptester.pri)

symbian {
   qt_not_deployed {
      plugins.files = qjpeg.dll
      plugins.path = imageformats
      DEPLOYMENT += plugins
   }
}
