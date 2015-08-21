TEMPLATE = app

QT += svg testlib
qtHaveModule(opengl): QT += opengl

HEADERS += widgets/gvbwidget.h \
        widgets/abstractscrollarea.h \
        widgets/mainview.h \
        widgets/iconitem.h \
        widgets/label.h \
        widgets/listitem.h \
        widgets/scrollbar.h \
        widgets/simplelistview.h \
        widgets/scroller.h \
        widgets/scroller_p.h \
        widgets/button.h \
        widgets/menu.h \
        widgets/themeevent.h \
        widgets/theme.h \
        widgets/backgrounditem.h \
        widgets/topbar.h \
        widgets/commandline.h \
        widgets/dummydatagen.h \
        widgets/settings.h \
        widgets/listitemcache.h \
        widgets/listwidget.h \
        widgets/simplelist.h \
        widgets/itemrecyclinglist.h \
        widgets/itemrecyclinglistview.h \
        widgets/abstractitemview.h \
        widgets/abstractviewitem.h \
        widgets/recycledlistitem.h \
        widgets/listitemcontainer.h \
        widgets/abstractitemcontainer.h \
        widgets/listmodel.h

SOURCES += main.cpp \
        widgets/gvbwidget.cpp \
        widgets/abstractscrollarea.cpp \
        widgets/mainview.cpp \
        widgets/iconitem.cpp \
        widgets/label.cpp \
        widgets/listitem.cpp \
        widgets/scrollbar.cpp \
        widgets/simplelistview.cpp \
        widgets/scroller.cpp \
        widgets/button.cpp \
        widgets/menu.cpp \
        widgets/themeevent.cpp \
        widgets/theme.cpp \
        widgets/backgrounditem.cpp \
        widgets/topbar.cpp \
        widgets/commandline.cpp \
        widgets/dummydatagen.cpp \
        widgets/settings.cpp \
        widgets/listitemcache.cpp \
        widgets/listwidget.cpp \
        widgets/simplelist.cpp \
        widgets/itemrecyclinglist.cpp \
        widgets/itemrecyclinglistview.cpp \
        widgets/abstractitemview.cpp \
        widgets/abstractviewitem.cpp \
        widgets/recycledlistitem.cpp \
        widgets/listitemcontainer.cpp \
        widgets/abstractitemcontainer.cpp \
        widgets/listmodel.cpp

TARGET = tst_GraphicsViewBenchmark
RESOURCES += GraphicsViewBenchmark.qrc
INCLUDEPATH += widgets
