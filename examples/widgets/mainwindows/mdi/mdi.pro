HEADERS       = mainwindow.h \
                mdichild.h
SOURCES       = main.cpp \
                mainwindow.cpp \
                mdichild.cpp
RESOURCES     = mdi.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/mainwindows/mdi
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS mdi.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/mainwindows/mdi
INSTALLS += target sources

QT += widgets

simulator: warning(This example might not fully work on Simulator platform)
