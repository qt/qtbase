HEADERS += \
        mouse.h \
        graphicsview.h
SOURCES += \
	main.cpp \
        mouse.cpp \
        graphicsview.cpp

RESOURCES += \
	mice.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/touch/pinchzoom
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS pinchzoom.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/touch/pinchzoom
INSTALLS += target sources
QT += widgets

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example

symbian: warning(This example might not fully work on Symbian platform)
maemo5: warning(This example might not fully work on Maemo platform)
simulator: warning(This example might not fully work on Simulator platform)
