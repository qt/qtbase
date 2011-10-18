HEADERS       = imagecomposer.h
SOURCES       = imagecomposer.cpp \
                main.cpp
RESOURCES     = imagecomposition.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/painting/imagecomposition
sources.files = $$SOURCES $$HEADERS $$RESOURCES images *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/painting/imagecomposition
INSTALLS += target sources

QT += widgets

