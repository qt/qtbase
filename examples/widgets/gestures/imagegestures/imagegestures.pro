QT += widgets
requires(qtConfig(filedialog))

HEADERS   = imagewidget.h \
            mainwidget.h
SOURCES   = imagewidget.cpp \
            main.cpp \
            mainwidget.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/gestures/imagegestures
INSTALLS += target

