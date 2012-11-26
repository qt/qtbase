SOURCES += main.cpp fademessage.cpp
HEADERS += fademessage.h
RESOURCES += fademessage.qrc
INSTALLS += target sources

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/effects/fademessage
sources.files = $$SOURCES \
    $$HEADERS \
    $$RESOURCES \
    $$FORMS \
    fademessage.pro \
    background.jpg
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/effects/fademessage


QT += widgets
simulator: warning(This example might not fully work on Simulator platform)
