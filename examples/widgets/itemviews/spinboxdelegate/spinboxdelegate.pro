QT += widgets

HEADERS     = delegate.h
SOURCES     = delegate.cpp \
              main.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/itemviews/spinboxdelegate
INSTALLS += target
