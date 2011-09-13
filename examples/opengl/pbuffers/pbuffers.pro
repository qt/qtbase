HEADERS += glwidget.h \
    cube.h
SOURCES += glwidget.cpp \
    main.cpp \
    cube.cpp
RESOURCES += pbuffers.qrc
QT += opengl widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtbase/opengl/pbuffers
sources.files = $$SOURCES \
    $$HEADERS \
    $$RESOURCES \
    pbuffers.pro \
    *.png
sources.path = $$[QT_INSTALL_EXAMPLES]/qtbase/opengl/pbuffers
INSTALLS += target \
    sources

symbian:CONFIG += qt_example
maemo5: CONFIG += qt_example

symbian: warning(This example might not fully work on Symbian platform)
maemo5: warning(This example does not work on Maemo platform)
simulator: warning(This example might not fully work on Simulator platform)
