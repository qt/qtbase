load(qt_plugin)

QT += core-private gui-private platformsupport-private

CONFIG += qpa/genericunixfontdatabase

OTHER_FILES += $$PWD/android.json

INCLUDEPATH += $$PWD
INCLUDEPATH += $$PWD/../../../../3rdparty/android/src

SOURCES += $$PWD/androidplatformplugin.cpp \
           $$PWD/androidjnimain.cpp \
           $$PWD/androidjniinput.cpp \
           $$PWD/androidjnimenu.cpp \
           $$PWD/androidjniclipboard.cpp \
           $$PWD/qandroidplatformintegration.cpp \
           $$PWD/qandroidplatformservices.cpp \
           $$PWD/qandroidassetsfileenginehandler.cpp \
           $$PWD/qandroidinputcontext.cpp \
           $$PWD/qandroidplatformfontdatabase.cpp \
           $$PWD/qandroidplatformclipboard.cpp \
           $$PWD/qandroidplatformtheme.cpp \
           $$PWD/qandroidplatformmenubar.cpp \
           $$PWD/qandroidplatformmenu.cpp \
           $$PWD/qandroidplatformmenuitem.cpp


HEADERS += $$PWD/qandroidplatformintegration.h \
           $$PWD/androidjnimain.h \
           $$PWD/androidjniinput.h \
           $$PWD/androidjnimenu.h \
           $$PWD/androidjniclipboard.h \
           $$PWD/qandroidplatformservices.h \
           $$PWD/qandroidassetsfileenginehandler.h \
           $$PWD/qandroidinputcontext.h \
           $$PWD/qandroidplatformfontdatabase.h \
           $$PWD/qandroidplatformclipboard.h \
           $$PWD/qandroidplatformtheme.h \
           $$PWD/qandroidplatformmenubar.h \
           $$PWD/qandroidplatformmenu.h \
           $$PWD/qandroidplatformmenuitem.h


#Non-standard install directory, QTBUG-29859
DESTDIR = $$DESTDIR/android
target.path = $${target.path}/android
