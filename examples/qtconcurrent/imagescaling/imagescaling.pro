QT += concurrent widgets
requires(qtConfig(filedialog))

SOURCES += main.cpp imagescaling.cpp
HEADERS += imagescaling.h

target.path = $$[QT_INSTALL_EXAMPLES]/qtconcurrent/imagescaling
INSTALLS += target
