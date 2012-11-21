HEADERS       = mainwindow.h
SOURCES       = main.cpp \
                mainwindow.cpp
RESOURCES     = sdi.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/mainwindows/sdi
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS sdi.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/mainwindows/sdi
INSTALLS += target sources

QT += widgets

simulator: warning(This example might not fully work on Simulator platform)
