HEADERS += glwidget.h \
    cube.h
SOURCES += glwidget.cpp \
    main.cpp \
    cube.cpp
RESOURCES += pbuffers.qrc
QT += opengl

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
symbian:include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)
