SOURCES += main.cpp \
           dialog.cpp 

HEADERS += dialog.h

# Forms and resources
FORMS += dialog.ui

# install
target.path = $$[QT_INSTALL_EXAMPLES]/ipc/sharedmemory
sources.files = $$SOURCES $$HEADERS $$FORMS $$RESOURCES *.pro *.png
sources.path = $$[QT_INSTALL_EXAMPLES]/ipc/sharedmemory
INSTALLS += target sources

QT += widgets

simulator: warning(This example does not work on Simulator platform)
