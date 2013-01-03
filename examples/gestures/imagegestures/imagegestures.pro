QT += widgets

HEADERS   = imagewidget.h \
            mainwidget.h
SOURCES   = imagewidget.cpp \
            main.cpp \
            mainwidget.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/gestures/imagegestures
INSTALLS += target

