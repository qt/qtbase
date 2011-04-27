HEADERS       = renderarea.h \
                window.h
SOURCES       = main.cpp \
                renderarea.cpp \
                window.cpp
unix:!mac:!symbian:!vxworks:!integrity:LIBS += -lm

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/painting/painterpaths
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS painterpaths.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/painting/painterpaths
INSTALLS += target sources

symbian {
    TARGET.UID3 = 0xA000A64C
    include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
}
