QT += widgets

HEADERS     = slidersgroup.h \
              window.h
SOURCES     = main.cpp \
              slidersgroup.cpp \
              window.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/sliders
INSTALLS += target
