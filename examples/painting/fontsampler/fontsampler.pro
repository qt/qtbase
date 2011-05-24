FORMS     = mainwindowbase.ui
HEADERS   = mainwindow.h
SOURCES   = main.cpp \
            mainwindow.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/painting/fontsampler
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS fontsampler.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/painting/fontsampler
INSTALLS += target sources

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example
