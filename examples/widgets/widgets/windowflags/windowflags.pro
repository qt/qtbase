HEADERS       = controllerwindow.h \
                previewwindow.h
SOURCES       = controllerwindow.cpp \
                previewwindow.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/widgets/windowflags
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS windowflags.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/widgets/windowflags
INSTALLS += target sources

QT += widgets

simulator: warning(This example might not fully work on Simulator platform)
