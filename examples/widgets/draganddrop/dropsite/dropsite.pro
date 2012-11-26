HEADERS = droparea.h \
          dropsitewindow.h
SOURCES = droparea.cpp \
          dropsitewindow.cpp \
          main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/draganddrop/dropsite
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/draganddrop/dropsite
INSTALLS += target sources

QT += widgets
simulator: warning(This example might not fully work on Simulator platform)
