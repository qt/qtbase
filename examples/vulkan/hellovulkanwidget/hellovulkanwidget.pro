QT += widgets

HEADERS += \
    hellovulkanwidget.h \
    ../shared/trianglerenderer.h

SOURCES += \
    hellovulkanwidget.cpp \
    main.cpp \
    ../shared/trianglerenderer.cpp

RESOURCES += hellovulkanwidget.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/vulkan/hellovulkanwidget
INSTALLS += target
