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

DEPLOYMENT_PLUGIN += qjpeg

QT += widgets
