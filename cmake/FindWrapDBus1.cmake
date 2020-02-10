# DBus1 is buggy and breaks PKG_CONFIG environment.
# Work around that:-/
# See https://gitlab.freedesktop.org/dbus/dbus/issues/267 for more information

if(DBus1_FOUND OR WrapDBus1_FOUND)
    return()
endif()

if(DEFINED ENV{PKG_CONFIG_DIR})
    set(__qt_dbus_pcd "$ENV{PKG_CONFIG_DIR}")
endif()
if(DEFINED ENV{PKG_CONFIG_PATH})
    set(__qt_dbus_pcp "$ENV{PKG_CONFIG_PATH}")
endif()
if(DEFINED ENV{PKG_CONFIG_LIBDIR})
    set(__qt_dbus_pcl "$ENV{PKG_CONFIG_LIBDIR}")
endif()

find_package(DBus1)

if(DEFINED __qt_dbus_pcd)
    set(ENV{PKG_CONFIG_DIR} "${__qt_dbus_pcd}")
else()
    unset(ENV{PKG_CONFIG_DIR})
endif()
if(DEFINED __qt_dbus_pcp)
    set(ENV{PKG_CONFIG_PATH} "${__qt_dbus_pcp}")
else()
    unset(ENV{PKG_CONFIG_PATH})
endif()
if(DEFINED __qt_dbus_pcl)
    set(ENV{PKG_CONFIG_LIBDIR} "${__qt_dbus_pcl}")
else()
    unset(ENV{PKG_CONFIG_LIBDIR})
endif()

if(DBus1_FOUND)
    set(WrapDBus1_FOUND 1)
endif()
