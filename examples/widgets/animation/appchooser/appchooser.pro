SOURCES = main.cpp
RESOURCES = appchooser.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/animation/appchooser
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS appchooser.pro *.png
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/animation/appchooser
INSTALLS += target sources

QT += widgets
