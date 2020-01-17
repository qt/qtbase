TEMPLATE = app
QT += widgets opengl
requires(qtConfig(filedialog))

SOURCES += main.cpp \
           widget.cpp \
           renderwindow.cpp

HEADERS += widget.h \
           renderwindow.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/contextinfo
INSTALLS += target
