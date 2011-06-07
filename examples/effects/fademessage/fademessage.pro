SOURCES += main.cpp fademessage.cpp
HEADERS += fademessage.h
RESOURCES += fademessage.qrc
INSTALLS += target sources

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/effects/fademessage
sources.files = $$SOURCES \
    $$HEADERS \
    $$RESOURCES \
    $$FORMS \
    fademessage.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/effects/fademessage

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example

simulator: warning(This example might not fully work on Simulator platform)
