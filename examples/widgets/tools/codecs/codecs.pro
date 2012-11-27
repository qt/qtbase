HEADERS      += mainwindow.h \
                previewform.h
SOURCES      += main.cpp \
                mainwindow.cpp \
                previewform.cpp

EXAMPLE_FILES = encodedfiles

# install
target.path = $$[QT_INSTALL_EXAMPLES]/tools/codecs
INSTALLS += target

QT += widgets

simulator: warning(This example might not fully work on Simulator platform)
