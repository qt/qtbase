HEADERS = $$PWD/glwindow.h

SOURCES = $$PWD/glwindow.cpp \
          $$PWD/main.cpp

RESOURCES += computegles31.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/opengl/computegles31
INSTALLS += target
