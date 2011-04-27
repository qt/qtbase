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
