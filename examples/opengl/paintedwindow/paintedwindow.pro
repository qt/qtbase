HEADERS += paintedwindow.h
SOURCES += paintedwindow.cpp main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/paintedwindow
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS paintedwindow.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/opengl/paintedwindow
INSTALLS += target sources
