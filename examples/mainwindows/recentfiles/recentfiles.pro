HEADERS       = mainwindow.h
SOURCES       = main.cpp \
                mainwindow.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/mainwindows/recentfiles
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS recentfiles.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/mainwindows/recentfiles
INSTALLS += target sources

symbian: CONFIG += qt_example
QT += widgets
