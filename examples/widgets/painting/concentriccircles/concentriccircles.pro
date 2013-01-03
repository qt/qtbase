QT += widgets

HEADERS       = circlewidget.h \
                window.h
SOURCES       = circlewidget.cpp \
                main.cpp \
                window.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/painting/concentriccircles
INSTALLS += target
