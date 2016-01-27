QT += concurrent widgets

SOURCES += main.cpp imagescaling.cpp
HEADERS += imagescaling.h

target.path = $$[QT_INSTALL_EXAMPLES]/qtconcurrent/imagescaling
INSTALLS += target

wince: DEPLOYMENT_PLUGIN += qgif qjpeg
