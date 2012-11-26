HEADERS       = mainwindow.h \
                scribblearea.h
SOURCES       = main.cpp \
                mainwindow.cpp \
                scribblearea.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/scribble
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS scribble.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/scribble
INSTALLS += target sources

QT += widgets
!isEmpty(QT.printsupport.name): QT += printsupport
