QT += concurrent widgets
CONFIG += console

SOURCES += main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtconcurrent/progressdialog
sources.files = $$SOURCES $$HEADERS $$FORMS $$RESOURCES *.pro *.png
sources.path = $$[QT_INSTALL_EXAMPLES]/qtconcurrent/progressdialog
INSTALLS += target sources

simulator: warning(This example does not work on Simulator platform)
