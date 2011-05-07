HEADERS       = stardelegate.h \
                stareditor.h \
                starrating.h
SOURCES       = main.cpp \
                stardelegate.cpp \
                stareditor.cpp \
                starrating.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/itemviews/stardelegate
sources.files = $$SOURCES $$HEADERS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/itemviews/stardelegate
INSTALLS += target sources

symbian: CONFIG += qt_example

QT += widgets
