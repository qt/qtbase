SOURCES = main.cpp
RESOURCES = appchooser.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/animation/appchooser
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS appchooser.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/animation/appchooser
INSTALLS += target sources

QT += widgets
