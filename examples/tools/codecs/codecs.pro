HEADERS      += mainwindow.h \
                previewform.h
SOURCES      += main.cpp \
                mainwindow.cpp \
                previewform.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/tools/codecs
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS encodedfiles codecs.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/tools/codecs
INSTALLS += target sources

QT += widgets

simulator: warning(This example might not fully work on Simulator platform)
