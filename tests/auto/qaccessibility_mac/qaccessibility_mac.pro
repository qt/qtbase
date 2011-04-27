CONFIG += qttest_p4 
SOURCES += tst_qaccessibility_mac.cpp
TARGET = tst_qaccessibility_mac

RESOURCES = qaccessibility_mac.qrc

requires(mac)

# this setup should support both building as an autotest
# (where uilib isn't built by default), and when running shadow
# builds (where QTDIR points to the build directory).
# autotest + shadow build is not supported :)
exists($$(QTDIR)/tools/designer/src/lib/uilib/uilib.pri) {
   include($$(QTDIR)/tools/designer/src/lib/uilib/uilib.pri, "", true)
   INCLUDEPATH += $$(QTDIR)/tools/designer/src/uitools
   SOURCES += $$(QTDIR)/tools/designer/src/uitools/quiloader.cpp
   HEADERS += $$(QTDIR)/tools/designer/src/uitools/quiloader.h
} else {
    CONFIG += uitools
}
QT += xml
LIBS += -framework ApplicationServices -framework Carbon

