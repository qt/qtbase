TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += .

# Input
SOURCES += main.cpp imagescaling.cpp
HEADERS += imagescaling.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/qtconcurrent/imagescaling
sources.files = $$SOURCES $$HEADERS $$FORMS $$RESOURCES *.pro *.png
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/qtconcurrent/imagescaling
INSTALLS += target sources


wince*: DEPLOYMENT_PLUGIN += qgif qjpeg qtiff
QT += widgets

simulator: warning(This example does not work on Simulator platform)
