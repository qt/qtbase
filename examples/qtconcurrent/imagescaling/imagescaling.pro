QT += concurrent widgets

SOURCES += main.cpp imagescaling.cpp
HEADERS += imagescaling.h

target.path = $$[QT_INSTALL_EXAMPLES]/qtconcurrent/imagescaling
sources.files = $$SOURCES $$HEADERS $$FORMS $$RESOURCES *.pro *.png
sources.path = $$[QT_INSTALL_EXAMPLES]/qtconcurrent/imagescaling
INSTALLS += target sources

wince*: DEPLOYMENT_PLUGIN += qgif qjpeg

simulator: warning(This example does not work on Simulator platform)
