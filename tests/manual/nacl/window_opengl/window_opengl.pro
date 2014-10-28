HEADERS += hellowindow.h
SOURCES += hellowindow.cpp main.cpp

# should be set by Qt, but isn't
INCLUDEPATH += $$(NACL_SDK_ROOT)/include

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/hellowindow
INSTALLS += target

