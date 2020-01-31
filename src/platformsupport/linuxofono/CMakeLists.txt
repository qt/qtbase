# Generated from linuxofono.pro.

#####################################################################
## LinuxOfonoSupport Module:
#####################################################################

add_qt_module(LinuxOfonoSupport
    STATIC
    INTERNAL_MODULE
    SOURCES
        qofonoservice_linux.cpp qofonoservice_linux_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::DBus
)

#### Keys ignored in scope 1:.:.:linuxofono.pro:<TRUE>:
# MODULE = "linuxofono_support"
