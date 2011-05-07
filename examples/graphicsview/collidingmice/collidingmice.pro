HEADERS += \
	mouse.h
SOURCES += \
	main.cpp \
        mouse.cpp

RESOURCES += \
	mice.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/graphicsview/collidingmice
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS collidingmice.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/graphicsview/collidingmice
INSTALLS += target sources

symbian {
    TARGET.UID3 = 0xA000A643
    CONFIG += qt_example
}
QT += widgets
