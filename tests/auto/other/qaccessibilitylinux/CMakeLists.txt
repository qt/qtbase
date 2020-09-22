# Generated from qaccessibilitylinux.pro.
# special case begin
if (NOT QT_FEATURE_accessibility OR NOT TARGET Qt::LinuxAccessibilitySupportPrivate
        OR (QT_BUILD_STANDALONE_TESTS AND QT_WILL_INSTALL))
    return()
endif()
# special case end

#####################################################################
## tst_qaccessibilitylinux Test:
#####################################################################

qt_internal_add_test(tst_qaccessibilitylinux
    SOURCES
        tst_qaccessibilitylinux.cpp
    DBUS_INTERFACE_SOURCES
        ../../../../src/platformsupport/linuxaccessibility/dbusxml/Bus.xml
    PUBLIC_LIBRARIES
        Qt::DBus
        Qt::Gui
        Qt::GuiPrivate
        Qt::LinuxAccessibilitySupportPrivate
        Qt::Widgets
)

# special case begin
# require for struct_marshallers_p.h which is included dbus_interface.h
target_include_directories(tst_qaccessibilitylinux PRIVATE
    ../../../../src/platformsupport/linuxaccessibility
)
# special case end
