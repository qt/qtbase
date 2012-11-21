HEADERS       = renderarea.h \
                window.h
SOURCES       = main.cpp \
                renderarea.cpp \
                window.cpp
unix:!mac:!vxworks:!integrity:LIBS += -lm

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/painting/painterpaths
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS painterpaths.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/widgets/painting/painterpaths
INSTALLS += target sources

QT += widgets

