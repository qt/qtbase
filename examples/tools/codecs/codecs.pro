HEADERS      += mainwindow.h \
                previewform.h
SOURCES      += main.cpp \
                mainwindow.cpp \
                previewform.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/codecs
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS encodedfiles codecs.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/tools/codecs
INSTALLS += target sources

symbian: CONFIG += qt_example
QT += widgets
