QT += widgets

HEADERS = window.h \
          animation.h
SOURCES = main.cpp \
          window.cpp

FORMS   = form.ui

RESOURCES = easing.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/animation/easing
INSTALLS += target
