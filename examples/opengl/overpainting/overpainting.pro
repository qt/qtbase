VPATH += ../shared
INCLUDEPATH += ../shared

QT += opengl widgets
HEADERS = bubble.h \
    glwidget.h \
    qtlogo.h
SOURCES = bubble.cpp \
    glwidget.cpp \
    main.cpp \
    qtlogo.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/overpainting
sources.files = $$SOURCES \
    $$HEADERS \
    $$RESOURCES \
    $$FORMS \
    overpainting.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/opengl/overpainting
INSTALLS += target \
    sources


simulator: warning(This example might not fully work on Simulator platform)
