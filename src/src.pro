TEMPLATE = subdirs

load(qfeatures)

src_qtzlib.file = $$PWD/corelib/qtzlib.pro
src_qtzlib.target = sub-zlib

src_tools_bootstrap.subdir = tools/bootstrap
src_tools_bootstrap.target = sub-bootstrap
src_tools_bootstrap.CONFIG = host_build

src_tools_moc.subdir = tools/moc
src_tools_moc.target = sub-moc
src_tools_moc.depends = src_tools_bootstrap
src_tools_moc.CONFIG = host_build

src_tools_rcc.subdir = tools/rcc
src_tools_rcc.target = sub-rcc
src_tools_rcc.depends = src_tools_bootstrap
src_tools_rcc.CONFIG = host_build

src_tools_qlalr.subdir = tools/qlalr
src_tools_qlalr.target = sub-qlalr
src_tools_qlalr.CONFIG = host_build
force_bootstrap: src_tools_qlalr.depends = src_tools_bootstrap
else: src_tools_qlalr.depends = src_corelib

src_tools_uic.subdir = tools/uic
src_tools_uic.target = sub-uic
src_tools_uic.CONFIG = host_build
force_bootstrap: src_tools_uic.depends = src_tools_bootstrap
else: src_tools_uic.depends = src_corelib

src_tools_bootstrap_dbus.subdir = tools/bootstrap-dbus
src_tools_bootstrap_dbus.target = sub-bootstrap_dbus
src_tools_bootstrap_dbus.depends = src_tools_bootstrap
src_tools_bootstrap_dbus.CONFIG = host_build

src_tools_qdbusxml2cpp.subdir = tools/qdbusxml2cpp
src_tools_qdbusxml2cpp.target = sub-qdbusxml2cpp
src_tools_qdbusxml2cpp.CONFIG = host_build
force_bootstrap: src_tools_qdbusxml2cpp.depends = src_tools_bootstrap_dbus
else: src_tools_qdbusxml2cpp.depends = src_dbus

src_tools_qdbuscpp2xml.subdir = tools/qdbuscpp2xml
src_tools_qdbuscpp2xml.target = sub-qdbuscpp2xml
src_tools_qdbuscpp2xml.CONFIG = host_build
force_bootstrap: src_tools_qdbuscpp2xml.depends = src_tools_bootstrap_dbus
else: src_tools_qdbuscpp2xml.depends = src_dbus

src_winmain.subdir = $$PWD/winmain
src_winmain.target = sub-winmain
src_winmain.depends = sub-corelib  # just for the module .pri file

src_corelib.subdir = $$PWD/corelib
src_corelib.target = sub-corelib
src_corelib.depends = src_tools_moc src_tools_rcc

src_xml.subdir = $$PWD/xml
src_xml.target = sub-xml
src_xml.depends = src_corelib

src_dbus.subdir = $$PWD/dbus
src_dbus.target = sub-dbus
src_dbus.depends = src_corelib
force_bootstrap: src_dbus.depends += src_tools_bootstrap_dbus  # avoid syncqt race

src_concurrent.subdir = $$PWD/concurrent
src_concurrent.target = sub-concurrent
src_concurrent.depends = src_corelib

src_sql.subdir = $$PWD/sql
src_sql.target = sub-sql
src_sql.depends = src_corelib

src_network.subdir = $$PWD/network
src_network.target = sub-network
src_network.depends = src_corelib

src_testlib.subdir = $$PWD/testlib
src_testlib.target = sub-testlib
src_testlib.depends = src_corelib   # testlib links only to corelib, but see below for the headers

src_3rdparty_pcre.subdir = $$PWD/3rdparty/pcre
src_3rdparty_pcre.target = sub-3rdparty-pcre

src_3rdparty_harfbuzzng.subdir = $$PWD/3rdparty/harfbuzz-ng
src_3rdparty_harfbuzzng.target = sub-3rdparty-harfbuzzng
src_3rdparty_harfbuzzng.depends = src_corelib   # for the Qt atomics

src_3rdparty_freetype.subdir = $$PWD/3rdparty/freetype
src_3rdparty_freetype.target = sub-3rdparty-freetype

src_angle.subdir = $$PWD/angle
src_angle.target = sub-angle

src_gui.subdir = $$PWD/gui
src_gui.target = sub-gui
src_gui.depends = src_corelib

src_platformheaders.subdir = $$PWD/platformheaders
src_platformheaders.target = sub-platformheaders
src_platformheaders.depends = src_corelib src_gui

src_platformsupport.subdir = $$PWD/platformsupport
src_platformsupport.target = sub-platformsupport
src_platformsupport.depends = src_corelib src_gui src_platformheaders

src_widgets.subdir = $$PWD/widgets
src_widgets.target = sub-widgets
src_widgets.depends = src_corelib src_gui src_tools_uic src_platformheaders

src_opengl.subdir = $$PWD/opengl
src_opengl.target = sub-opengl
src_opengl.depends = src_gui src_widgets

src_openglextensions.subdir = $$PWD/openglextensions
src_openglextensions.target = sub-openglextensions
src_openglextensions.depends = src_gui

src_printsupport.subdir = $$PWD/printsupport
src_printsupport.target = sub-printsupport
src_printsupport.depends = src_corelib src_gui src_widgets src_tools_uic

src_plugins.subdir = $$PWD/plugins
src_plugins.target = sub-plugins
src_plugins.depends = src_sql src_xml src_network

src_android.subdir = $$PWD/android

# this order is important
contains(QT_CONFIG, zlib)|cross_compile: SUBDIRS += src_qtzlib
SUBDIRS += src_tools_bootstrap src_tools_moc src_tools_rcc
!contains(QT_DISABLED_FEATURES, regularexpression):pcre {
    SUBDIRS += src_3rdparty_pcre
    src_corelib.depends += src_3rdparty_pcre
}
SUBDIRS += src_corelib src_tools_qlalr
TOOLS = src_tools_moc src_tools_rcc src_tools_qlalr
win32:SUBDIRS += src_winmain
SUBDIRS += src_network src_sql src_xml src_testlib
contains(QT_CONFIG, dbus) {
    force_bootstrap: SUBDIRS += src_tools_bootstrap_dbus
    SUBDIRS += src_dbus src_tools_qdbusxml2cpp src_tools_qdbuscpp2xml
    TOOLS += src_tools_qdbusxml2cpp src_tools_qdbuscpp2xml
    contains(QT_CONFIG, accessibility-atspi-bridge): \
        src_platformsupport.depends += src_dbus src_tools_qdbusxml2cpp
    src_plugins.depends += src_dbus src_tools_qdbusxml2cpp src_tools_qdbuscpp2xml
}
contains(QT_CONFIG, concurrent):SUBDIRS += src_concurrent
!contains(QT_CONFIG, no-gui) {
    contains(QT_CONFIG, harfbuzz) {
        SUBDIRS += src_3rdparty_harfbuzzng
        src_gui.depends += src_3rdparty_harfbuzzng
    }
    win32:contains(QT_CONFIG, angle)|contains(QT_CONFIG, dynamicgl) {
        SUBDIRS += src_angle
        src_gui.depends += src_angle
    }
    contains(QT_CONFIG, freetype) {
        SUBDIRS += src_3rdparty_freetype
        src_platformsupport.depends += src_3rdparty_freetype
    }
    SUBDIRS += src_gui src_platformsupport src_platformheaders
    contains(QT_CONFIG, opengl(es2)?):SUBDIRS += src_openglextensions
    src_plugins.depends += src_gui src_platformsupport src_platformheaders
    src_testlib.depends += src_gui      # if QtGui is enabled, QtTest requires QtGui's headers
    !contains(QT_CONFIG, no-widgets) {
        SUBDIRS += src_tools_uic src_widgets
        TOOLS += src_tools_uic
        src_plugins.depends += src_widgets
        src_testlib.depends += src_widgets        # if QtWidgets is enabled, QtTest requires QtWidgets's headers
        contains(QT_CONFIG, opengl(es2)?) {
            SUBDIRS += src_opengl
            src_plugins.depends += src_opengl
        }
        !wince:!winrt {
            SUBDIRS += src_printsupport
            src_plugins.depends += src_printsupport
        }
    }
}
SUBDIRS += src_plugins

nacl: SUBDIRS -= src_network src_testlib

android:!android-no-sdk: SUBDIRS += src_android

TR_EXCLUDE = \
    src_tools_bootstrap src_tools_moc src_tools_rcc src_tools_uic src_tools_qlalr \
    src_tools_bootstrap_dbus src_tools_qdbusxml2cpp src_tools_qdbuscpp2xml \
    src_3rdparty_pcre src_3rdparty_harfbuzzng src_3rdparty_freetype

sub-tools.depends = $$TOOLS
QMAKE_EXTRA_TARGETS = sub-tools
