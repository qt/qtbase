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
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/opengl/overpainting
sources.files = $$SOURCES \
    $$HEADERS \
    $$RESOURCES \
    $$FORMS \
    overpainting.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/opengl/overpainting
INSTALLS += target \
    sources

symbian:CONFIG += qt_example
maemo5: CONFIG += qt_example

symbian: warning(This example might not fully work on Symbian platform)
maemo5: warning(This example does not work on Maemo platform)
simulator: warning(This example might not fully work on Simulator platform)
