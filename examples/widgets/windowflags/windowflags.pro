HEADERS       = controllerwindow.h \
                previewwindow.h
SOURCES       = controllerwindow.cpp \
                previewwindow.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/windowflags
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS windowflags.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/windowflags
INSTALLS += target sources

symbian: CONFIG += qt_example
QT += widgets
