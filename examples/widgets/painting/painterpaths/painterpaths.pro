QT += widgets
requires(qtConfig(combobox))

HEADERS       = renderarea.h \
                window.h
SOURCES       = main.cpp \
                renderarea.cpp \
                window.cpp
unix:!mac:!vxworks:!integrity:!haiku:LIBS += -lm

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/painting/painterpaths
INSTALLS += target
