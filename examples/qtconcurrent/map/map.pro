TEMPLATE = app
TARGET = mapdemo
QT += concurrent
CONFIG += console
CONFIG -= app_bundle

SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/qtconcurrent/map
sources.files = $$SOURCES $$HEADERS $$FORMS $$RESOURCES *.pro *.png
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/qtconcurrent/map
INSTALLS += target sources

simulator: warning(This example does not work on Simulator platform)
