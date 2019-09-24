TARGET = qtforandroid

LIBS += -ljnigraphics -landroid

QT += \
    core-private gui-private \
    eventdispatcher_support-private accessibility_support-private \
    fontdatabase_support-private egl_support-private

qtConfig(vulkan): QT += vulkan_support-private

OTHER_FILES += $$PWD/android.json

INCLUDEPATH += \
    $$PWD \
    $$QT_SOURCE_TREE/src/3rdparty/android

SOURCES += $$PWD/main.cpp \
           $$PWD/androidplatformplugin.cpp \
           $$PWD/androidcontentfileengine.cpp \
           $$PWD/androiddeadlockprotector.cpp \
           $$PWD/androidjnimain.cpp \
           $$PWD/androidjniaccessibility.cpp \
           $$PWD/androidjniinput.cpp \
           $$PWD/androidjnimenu.cpp \
           $$PWD/androidjniclipboard.cpp \
           $$PWD/qandroidplatformintegration.cpp \
           $$PWD/qandroidplatformservices.cpp \
           $$PWD/qandroidassetsfileenginehandler.cpp \
           $$PWD/qandroidinputcontext.cpp \
           $$PWD/qandroidplatformaccessibility.cpp \
           $$PWD/qandroidplatformfontdatabase.cpp \
           $$PWD/qandroidplatformdialoghelpers.cpp \
           $$PWD/qandroidplatformclipboard.cpp \
           $$PWD/qandroidplatformtheme.cpp \
           $$PWD/qandroidplatformmenubar.cpp \
           $$PWD/qandroidplatformmenu.cpp \
           $$PWD/qandroidplatformmenuitem.cpp \
           $$PWD/qandroidsystemlocale.cpp \
           $$PWD/qandroidplatformscreen.cpp \
           $$PWD/qandroidplatformwindow.cpp \
           $$PWD/qandroidplatformopenglwindow.cpp \
           $$PWD/qandroidplatformbackingstore.cpp \
           $$PWD/qandroidplatformopenglcontext.cpp \
           $$PWD/qandroidplatformforeignwindow.cpp \
           $$PWD/qandroideventdispatcher.cpp \
           $$PWD/qandroidplatformoffscreensurface.cpp \
           $$PWD/qandroidplatformfiledialoghelper.cpp

HEADERS += $$PWD/qandroidplatformintegration.h \
           $$PWD/androidcontentfileengine.h \
           $$PWD/androiddeadlockprotector.h \
           $$PWD/androidjnimain.h \
           $$PWD/androidjniaccessibility.h \
           $$PWD/androidjniinput.h \
           $$PWD/androidjnimenu.h \
           $$PWD/androidjniclipboard.h \
           $$PWD/qandroidplatformservices.h \
           $$PWD/qandroidassetsfileenginehandler.h \
           $$PWD/qandroidinputcontext.h \
           $$PWD/qandroidplatformaccessibility.h \
           $$PWD/qandroidplatformfontdatabase.h \
           $$PWD/qandroidplatformclipboard.h \
           $$PWD/qandroidplatformdialoghelpers.h \
           $$PWD/qandroidplatformtheme.h \
           $$PWD/qandroidplatformmenubar.h \
           $$PWD/qandroidplatformmenu.h \
           $$PWD/qandroidplatformmenuitem.h \
           $$PWD/qandroidsystemlocale.h \
           $$PWD/androidsurfaceclient.h \
           $$PWD/qandroidplatformscreen.h \
           $$PWD/qandroidplatformwindow.h \
           $$PWD/qandroidplatformopenglwindow.h \
           $$PWD/qandroidplatformbackingstore.h \
           $$PWD/qandroidplatformopenglcontext.h \
           $$PWD/qandroidplatformforeignwindow.h \
           $$PWD/qandroideventdispatcher.h \
           $$PWD/qandroidplatformoffscreensurface.h \
           $$PWD/qandroidplatformfiledialoghelper.h

qtConfig(android-style-assets): SOURCES += $$PWD/extract.cpp
else: SOURCES += $$PWD/extract-dummy.cpp

qtConfig(vulkan) {
    SOURCES += $$PWD/qandroidplatformvulkaninstance.cpp \
               $$PWD/qandroidplatformvulkanwindow.cpp
    HEADERS += $$PWD/qandroidplatformvulkaninstance.h \
               $$PWD/qandroidplatformvulkanwindow.h
}

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QAndroidIntegrationPlugin
load(qt_plugin)
