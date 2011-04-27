HEADERS       = mainwindow.h
SOURCES       = main.cpp \
                mainwindow.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/mainwindows/recentfiles
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS recentfiles.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/mainwindows/recentfiles
INSTALLS += target sources

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example

simulator: warning(This example might not fully work on Simulator platform)
