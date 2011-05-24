HEADERS += calibration.h \
	   scribblewidget.h
SOURCES += calibration.cpp \
	   scribblewidget.cpp \
	   main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/qws/mousecalibration
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/qws/mousecalibration
INSTALLS += target sources
QT += widgets

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
maemo5: include($$QT_SOURCE_TREE/examples/maemo5pkgrules.pri)

symbian: warning(This example does not work on Symbian platform)
maemo5: warning(This example might not fully work on Maemo platform)
simulator: warning(This example does not work on Simulator platform)
