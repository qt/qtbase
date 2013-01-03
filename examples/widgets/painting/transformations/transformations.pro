QT += widgets

HEADERS     = renderarea.h \
              window.h
SOURCES     = main.cpp \
              renderarea.cpp \
	      window.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/painting/transformations
INSTALLS += target
