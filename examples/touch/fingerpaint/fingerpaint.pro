HEADERS       = mainwindow.h \
                scribblearea.h
SOURCES       = main.cpp \
                mainwindow.cpp \
                scribblearea.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/touch/fingerpaint
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS fingerpaint.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/touch/fingerpaint
INSTALLS += target sources
