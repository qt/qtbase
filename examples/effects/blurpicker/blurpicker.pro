SOURCES += main.cpp blurpicker.cpp blureffect.cpp
HEADERS += blurpicker.h blureffect.h
RESOURCES += blurpicker.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/effects/blurpicker
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS blurpicker.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/effects/blurpicker
INSTALLS += target sources

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example
