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
