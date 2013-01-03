QT += widgets

HEADERS       = analogclock.h
SOURCES       = analogclock.cpp \
                main.cpp

QMAKE_PROJECT_NAME = widgets_analogclock

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/analogclock
INSTALLS += target
