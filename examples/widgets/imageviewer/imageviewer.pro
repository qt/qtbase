HEADERS       = imageviewer.h
SOURCES       = imageviewer.cpp \
                main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/imageviewer
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS imageviewer.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/imageviewer
INSTALLS += target sources


#Symbian has built-in component named imageviewer so we use different target

wince*: {
   DEPLOYMENT_PLUGIN += qjpeg qgif
}
QT += widgets printsupport

simulator: warning(This example might not fully work on Simulator platform)
