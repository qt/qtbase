QT += widgets
requires(qtConfig(filedialog))

HEADERS	    =	mainwindow.h \
        tabletcanvas.h \
        tabletapplication.h
SOURCES	    =	mainwindow.cpp \
        main.cpp \
        tabletcanvas.cpp \
        tabletapplication.cpp
RESOURCES += images.qrc

# Avoid naming the target "tablet", as it would create an executable
# named "tablet.exe" on Windows and trigger a bug (in the Wacom drivers, apparently)
# preventing tablet messages from being received.
TARGET = qttablet

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/tablet
INSTALLS += target
