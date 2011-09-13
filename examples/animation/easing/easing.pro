HEADERS = window.h \
          animation.h
SOURCES = main.cpp \
          window.cpp

FORMS   = form.ui

RESOURCES = easing.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/animation/easing
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS easing.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/animation/easing
INSTALLS += target sources

symbian {
    TARGET.UID3 = 0xA000E3F6
    CONFIG += qt_example
}
QT += widgets

maemo5: CONFIG += qt_example
