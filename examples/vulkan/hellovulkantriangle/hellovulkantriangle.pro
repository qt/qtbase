HEADERS += \
    trianglerenderer.h

SOURCES += \
    main.cpp \
    trianglerenderer.cpp

RESOURCES += hellovulkantriangle.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/vulkan/hellovulkantriangle
INSTALLS += target
