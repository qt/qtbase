HEADERS = droparea.h \
          dropsitewindow.h
SOURCES = droparea.cpp \
          dropsitewindow.cpp \
          main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/draganddrop/dropsite
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/draganddrop/dropsite
INSTALLS += target sources

symbian: CONFIG += qt_example
QT += widgets
