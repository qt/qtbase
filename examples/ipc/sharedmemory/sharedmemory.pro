QT += widgets

SOURCES += main.cpp \
           dialog.cpp 

HEADERS += dialog.h

# Forms and resources
FORMS += dialog.ui

EXAMPLE_FILES = *.png

# install
target.path = $$[QT_INSTALL_EXAMPLES]/ipc/sharedmemory
INSTALLS += target
