HEADERS       = mainwindow.h \
                scribblearea.h
SOURCES       = main.cpp \
                mainwindow.cpp \
                scribblearea.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/scribble
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS scribble.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/scribble
INSTALLS += target sources

symbian: CONFIG += qt_example
maemo5: CONFIG += qt_example
