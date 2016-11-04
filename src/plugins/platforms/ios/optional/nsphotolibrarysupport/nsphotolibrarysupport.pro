TARGET = qiosnsphotolibrarysupport

# QTBUG-42937: Since the iOS plugin (kernel) is
# static, this plugin needs to be static as well.
qtConfig(shared): CONFIG += static

QT += core gui gui-private
LIBS += -framework Foundation -framework UIKit -framework AssetsLibrary

HEADERS = \
        qiosfileengineassetslibrary.h \
        qiosfileenginefactory.h \
        qiosimagepickercontroller.h

OBJECTIVE_SOURCES = \
        plugin.mm \
        qiosfileengineassetslibrary.mm \
        qiosimagepickercontroller.mm \

OTHER_FILES = \
        plugin.json

PLUGIN_CLASS_NAME = QIosOptionalPlugin_NSPhotoLibrary
PLUGIN_EXTENDS = -
PLUGIN_TYPE = platforms/darwin
load(qt_plugin)
