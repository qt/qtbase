QT += widgets
qtHaveModule(opengl): QT += opengl
HEADERS += boat.h \
    bomb.h \
    mainwindow.h \
    submarine.h \
    torpedo.h \
    pixmapitem.h \
    graphicsscene.h \
    animationmanager.h \
    states.h \
    boat_p.h \
    submarine_p.h \
    qanimationstate.h \
    progressitem.h \
    textinformationitem.h
SOURCES += boat.cpp \
    bomb.cpp \
    main.cpp \
    mainwindow.cpp \
    submarine.cpp \
    torpedo.cpp \
    pixmapitem.cpp \
    graphicsscene.cpp \
    animationmanager.cpp \
    states.cpp \
    qanimationstate.cpp \
    progressitem.cpp \
    textinformationitem.cpp
RESOURCES += subattaq.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/animation/sub-attaq
INSTALLS += target
