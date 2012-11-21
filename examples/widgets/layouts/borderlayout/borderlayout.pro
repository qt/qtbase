HEADERS     = borderlayout.h \
              window.h
SOURCES     = borderlayout.cpp \
              main.cpp \
              window.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/layouts/borderlayout
sources.files = $$SOURCES $$HEADERS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/layouts/borderlayout
INSTALLS += target sources

QT += widgets
