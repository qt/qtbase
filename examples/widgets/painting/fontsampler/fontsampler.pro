FORMS     = mainwindowbase.ui
HEADERS   = mainwindow.h
SOURCES   = main.cpp \
            mainwindow.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/painting/fontsampler
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS fontsampler.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/painting/fontsampler
INSTALLS += target sources

QT += widgets
!isEmpty(QT.printsupport.name): QT += printsupport
