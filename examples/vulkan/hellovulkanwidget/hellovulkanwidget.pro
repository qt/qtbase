QT += widgets

HEADERS += \
    hellovulkanwidget.h \
    trianglerenderer.h

SOURCES += \
    hellovulkanwidget.cpp \
    main.cpp \
    trianglerenderer.cpp

RESOURCES += hellovulkanwidget.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/vulkan/hellovulkanwidget
INSTALLS += target
