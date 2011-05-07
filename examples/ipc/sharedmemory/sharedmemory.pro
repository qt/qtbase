SOURCES += main.cpp \
           dialog.cpp 

HEADERS += dialog.h

# Forms and resources
FORMS += dialog.ui

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/ipc/sharedmemory
sources.files = $$SOURCES $$HEADERS $$FORMS $$RESOURCES *.pro *.png
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/ipc/sharedmemory
INSTALLS += target sources

symbian: CONFIG += qt_example
QT += widgets
