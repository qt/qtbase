HEADERS       = mainwindow.h \
                scribblearea.h
SOURCES       = main.cpp \
                mainwindow.cpp \
                scribblearea.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/touch/fingerpaint
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS fingerpaint.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/touch/fingerpaint
INSTALLS += target sources

QT += widgets
!isEmpty(QT.printsupport.name): QT += printsupport

simulator: warning(This example might not fully work on Simulator platform)
