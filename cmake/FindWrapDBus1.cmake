# DBus1 is buggy and breaks PKG_CONFIG environment.
# Work around that:-/

set(__qt_dbus_pcd $ENV{PKG_CONFIG_DIR})
set(__qt_dbus_pcp $ENV{PKG_CONFIG_PATH})
set(__qt_dbus_pcl $ENV{PKG_CONFIG_LIBDIR})
find_package(DBus1)
set(ENV{PKG_CONFIG_DIR} ${__qt_dbus_pcd})
set(ENV{PKG_CONFIG_PATH} ${__qt_dbus_pcp})
set(ENV{PKG_CONFIG_LIBDIR} ${__qt_dbus_pcl})
