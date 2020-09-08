accessibility_adaptors.files = accessible/linux/dbusxml/Cache.xml accessible/linux/dbusxml/DeviceEventController.xml
accessibility_adaptors.header_flags = -i QtGui/private/qspi_struct_marshallers_p.h
DBUS_ADAPTORS += accessibility_adaptors

accessibility_interfaces.files = accessible/linux/dbusxml/Socket.xml accessible/linux/dbusxml/Bus.xml
accessibility_interfaces.header_flags = -i QtGui/private/qspi_struct_marshallers_p.h
DBUS_INTERFACES += accessibility_interfaces

HEADERS += \
    accessible/linux/atspiadaptor_p.h \
    accessible/linux/dbusconnection_p.h \
    accessible/linux/qspi_constant_mappings_p.h \
    accessible/linux/qspi_struct_marshallers_p.h \
    accessible/linux/qspiaccessiblebridge_p.h \
    accessible/linux/qspiapplicationadaptor_p.h \
    accessible/linux/qspidbuscache_p.h

SOURCES += \
    accessible/linux/atspiadaptor.cpp \
    accessible/linux/dbusconnection.cpp \
    accessible/linux/qspi_constant_mappings.cpp \
    accessible/linux/qspi_struct_marshallers.cpp \
    accessible/linux/qspiaccessiblebridge.cpp \
    accessible/linux/qspiapplicationadaptor.cpp \
    accessible/linux/qspidbuscache.cpp

QMAKE_USE_PRIVATE += atspi/nolink
