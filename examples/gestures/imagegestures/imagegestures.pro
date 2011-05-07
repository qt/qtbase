HEADERS   = imagewidget.h \
            mainwidget.h
SOURCES   = imagewidget.cpp \
            main.cpp \
            mainwidget.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/gestures/imagegestures
sources.files = $$SOURCES \
    $$HEADERS \
    $$RESOURCES \
    $$FORMS \
    imagegestures.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/gestures/imagegestures
INSTALLS += target \
    sources

symbian {
    TARGET.UID3 = 0xA000D7D0
    CONFIG += qt_example
}
QT += widgets
