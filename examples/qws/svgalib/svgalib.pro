TEMPLATE = lib
CONFIG += plugin

LIBS += -lvgagl -lvga

TARGET = svgalibscreen
target.path = $$[QT_INSTALL_PLUGINS]/gfxdrivers
INSTALLS += target

HEADERS	= svgalibscreen.h \
          svgalibpaintengine.h \
          svgalibsurface.h \
          svgalibpaintdevice.h
SOURCES	= svgalibscreen.cpp \
          svgalibpaintengine.cpp \
          svgalibsurface.cpp \
          svgalibpaintdevice.cpp \
          svgalibplugin.cpp

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example

symbian: warning(This example does not work on Symbian platform)
maemo5: warning(This example might not fully work on Maemo platform)
simulator: warning(This example does not work on Simulator platform)
