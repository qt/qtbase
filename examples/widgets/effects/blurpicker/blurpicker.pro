SOURCES += main.cpp blurpicker.cpp blureffect.cpp
HEADERS += blurpicker.h blureffect.h
RESOURCES += blurpicker.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/effects/blurpicker
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS blurpicker.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/effects/blurpicker
INSTALLS += target sources
QT += widgets

