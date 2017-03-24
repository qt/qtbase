QT += widgets concurrent

HEADERS += \
    mainwindow.h \
    vulkanwindow.h \
    renderer.h \
    mesh.h \
    shader.h \
    camera.h

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    vulkanwindow.cpp \
    renderer.cpp \
    mesh.cpp \
    shader.cpp \
    camera.cpp

RESOURCES += hellovulkancubes.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/vulkan/hellovulkancubes
INSTALLS += target
