# Skip Private module warnings that seep into stderr on static linux builds
# via Qt6GuiPlugins -> Qt6QEglFSEmulatorIntegrationPluginConfig
# -> Qt6EglFSDeviceIntegrationPrivate -> Qt6OpenGLPrivate
set(QT_NO_PRIVATE_MODULE_WARNING ON)

# This should successfully find the Qt6::BundledZLIB package in a -qt-zlib
# configuration.
find_package(Qt6 COMPONENTS Core)

# Depending on which module features were used when building Qt, look up each of the respective
# packages, those might also look for bundled libraries.
if(HAS_GUI)
    find_package(Qt6 COMPONENTS Gui)
endif()
if(HAS_DBUS)
    find_package(Qt6 COMPONENTS DBus)
endif()
if(HAS_WIDGETS)
    find_package(Qt6 COMPONENTS Widgets)
endif()
if(HAS_OPENGL)
    find_package(Qt6 COMPONENTS OpenGL)
endif()

