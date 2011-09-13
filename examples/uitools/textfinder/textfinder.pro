CONFIG		+= uitools
HEADERS		= textfinder.h
RESOURCES	= textfinder.qrc
SOURCES		= textfinder.cpp main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/uitools/textfinder
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro forms
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/uitools/textfinder
INSTALLS += target sources

symbian: CONFIG += qt_example
QT += widgets
maemo5: CONFIG += qt_example

symbian: warning(This example does not work on Symbian platform)
simulator: warning(This example does not work on Simulator platform)
