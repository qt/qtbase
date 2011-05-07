HEADERS       = imageviewer.h
SOURCES       = imageviewer.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/imageviewer
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS imageviewer.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/imageviewer
INSTALLS += target sources

symbian: CONFIG += qt_example

wince*: {
   DEPLOYMENT_PLUGIN += qjpeg qmng qgif
}
QT += widgets
