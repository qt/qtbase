HEADERS += glwidget.h \
    cube.h
SOURCES += glwidget.cpp \
    main.cpp \
    cube.cpp
RESOURCES += pbuffers.qrc
QT += opengl widgets

# install
target.path = $$[QT_INSTALL_EXAMPLES]/opengl/legacy/pbuffers
INSTALLS += target

contains(QT_CONFIG, opengles.|angle|dynamicgl):error("This example requires Qt to be configured with -opengl desktop")
