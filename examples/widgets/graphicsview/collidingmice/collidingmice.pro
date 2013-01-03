QT += widgets

HEADERS += \
	mouse.h
SOURCES += \
	main.cpp \
        mouse.cpp

RESOURCES += \
	mice.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/graphicsview/collidingmice
INSTALLS += target
