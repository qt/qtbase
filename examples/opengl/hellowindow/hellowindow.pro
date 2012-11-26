QT += gui-private core-private

HEADERS += hellowindow.h
SOURCES += hellowindow.cpp main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/hellowindow
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS hellowindow.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/opengl/hellowindow
INSTALLS += target sources

