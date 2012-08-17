HEADERS     = mainwindow.h \
              pieview.h
RESOURCES   = chart.qrc
SOURCES     = main.cpp \
              mainwindow.cpp \
              pieview.cpp
unix:!mac:!vxworks:!integrity:LIBS+= -lm

TARGET.EPOCHEAPSIZE = 0x200000 0x800000

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/itemviews/chart
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro *.cht
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/itemviews/chart
INSTALLS += target sources

QT += widgets

