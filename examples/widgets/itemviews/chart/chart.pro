QT += widgets

HEADERS     = mainwindow.h \
              pieview.h
RESOURCES   = chart.qrc
SOURCES     = main.cpp \
              mainwindow.cpp \
              pieview.cpp
unix:!mac:!vxworks:!integrity:LIBS+= -lm

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/itemviews/chart
INSTALLS += target
