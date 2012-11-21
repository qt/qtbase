HEADERS       = imageviewer.h
SOURCES       = imageviewer.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/widgets/imageviewer
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS imageviewer.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/widgets/imageviewer
INSTALLS += target sources


wince*: {
   DEPLOYMENT_PLUGIN += qjpeg qgif
}
QT += widgets
!isEmpty(QT.printsupport.name): QT += printsupport

simulator: warning(This example might not fully work on Simulator platform)
