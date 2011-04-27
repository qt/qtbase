HEADERS     = dialog.h
SOURCES     = dialog.cpp \
              main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/layouts/basiclayouts
sources.files = $$SOURCES $$HEADERS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/layouts/basiclayouts
INSTALLS += target sources

symbian: CONFIG += qt_example
