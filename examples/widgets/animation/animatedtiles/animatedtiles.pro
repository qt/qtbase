SOURCES = main.cpp
RESOURCES = animatedtiles.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/animation/animatedtiles
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS animatedtiles.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/animation/animatedtiles
INSTALLS += target sources

QT += widgets
