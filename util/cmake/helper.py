#############################################################################
##
## Copyright (C) 2018 The Qt Company Ltd.
## Contact: https://www.qt.io/licensing/
##
## This file is part of the plugins of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:GPL-EXCEPT$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see https://www.qt.io/terms-conditions. For further
## information use the contact form at https://www.qt.io/contact-us.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 3 as published by the Free Software
## Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
## included in the packaging of this file. Please review the following
## information to ensure the GNU General Public License requirements will
## be met: https://www.gnu.org/licenses/gpl-3.0.html.
##
## $QT_END_LICENSE$
##
#############################################################################

import re
import typing

class LibraryMapping:
    def __init__(self, soName: typing.Optional[str],
                 packageName: str, targetName: str, *,
                 resultVariable: typing.Optional[str] = None,
                 extra: typing.List[str] = [],
                 appendFoundSuffix: bool = True) -> None:
        self.soName = soName
        self.packageName = packageName
        self.resultVariable = resultVariable
        self.appendFoundSuffix = appendFoundSuffix
        self.extra = extra
        self.targetName = targetName

    def is_qt() -> bool:
        return self.packageName == 'Qt' \
            or self.packageName == 'Qt5' \
            or self.packageName == 'Qt6'

_qt_library_map = [
    # Qt:
    LibraryMapping('accessibility_support', 'Qt5', 'Qt::AccessibilitySupport', extra = ['COMPONENTS', 'AccessibilitySupport']),
    LibraryMapping('androidextras', 'Qt5', 'Qt::AndroidExtras', extra = ['COMPONENTS', 'AndroidExtras']),
    LibraryMapping('animation', 'Qt5', 'Qt::3DAnimation', extra = ['COMPONENTS', '3DAnimation']),
    LibraryMapping('application-lib', 'Qt5', 'Qt::AppManApplication', extra = ['COMPONENTS', 'AppManApplication']),
    LibraryMapping('bluetooth', 'Qt5', 'Qt::Bluetooth', extra = ['COMPONENTS', 'Bluetooth']),
    LibraryMapping('bootstrap', 'Qt5', 'Qt::Bootstrap', extra = ['COMPONENTS', 'Bootstrap']),
    # bootstrap-dbus: Not needed in Qt6!
    LibraryMapping('client', 'Qt5', 'Qt::WaylandClient', extra = ['COMPONENTS', 'WaylandClient']),
    LibraryMapping('clipboard_support', 'Qt5', 'Qt::ClipboardSupport', extra = ['COMPONENTS', 'ClipboardSupport']),
    LibraryMapping('common-lib', 'Qt5', 'Qt::AppManCommon', extra = ['COMPONENTS', 'AppManCommon']),
    LibraryMapping('compositor', 'Qt5', 'Qt::WaylandCompositor', extra = ['COMPONENTS', 'WaylandCompositor']),
    LibraryMapping('concurrent', 'Qt5', 'Qt::Concurrent', extra = ['COMPONENTS', 'Concurrent']),
    LibraryMapping('container', 'Qt5', 'Qt::AxContainer', extra = ['COMPONENTS', 'AxContainer']),
    LibraryMapping('control', 'Qt5', 'Qt::AxServer', extra = ['COMPONENTS', 'AxServer']),
    LibraryMapping('core_headers', 'Qt5', 'Qt::WebEngineCore', extra = ['COMPONENTS', 'WebEngineCore']),
    LibraryMapping('core', 'Qt5', 'Qt::Core', extra = ['COMPONENTS', 'Core']),
    LibraryMapping('coretest', 'Qt5', 'Qt::3DCoreTest', extra = ['COMPONENTS', '3DCoreTest']),
    LibraryMapping('crypto-lib', 'Qt5', 'Qt::AppManCrypto', extra = ['COMPONENTS', 'AppManCrypto']),
    LibraryMapping('dbus', 'Qt5', 'Qt::DBus', extra = ['COMPONENTS', 'DBus']),
    LibraryMapping('devicediscovery', 'Qt5', 'Qt::DeviceDiscoverySupport', extra = ['COMPONENTS', 'DeviceDiscoverySupport']),
    LibraryMapping('devicediscovery_support', 'Qt5', 'Qt::DeviceDiscoverySupport', extra = ['COMPONENTS', 'DeviceDiscoverySupport']),
    LibraryMapping('edid', 'Qt5', 'Qt::EdidSupport', extra = ['COMPONENTS', 'EdidSupport']),
    LibraryMapping('edid_support', 'Qt5', 'Qt::EdidSupport', extra = ['COMPONENTS', 'EdidSupport']),
    LibraryMapping('eglconvenience', 'Qt5', 'Qt::EglSupport', extra = ['COMPONENTS', 'EglSupport']),
    LibraryMapping('eglfsdeviceintegration', 'Qt5', 'Qt::EglFSDeviceIntegration', extra = ['COMPONENTS', 'EglFSDeviceIntegration']),
    LibraryMapping('eglfs_kms_support', 'Qt5', 'Qt::EglFsKmsSupport', extra = ['COMPONENTS', 'EglFsKmsSupport']),
    LibraryMapping('egl_support', 'Qt5', 'Qt::EglSupport', extra = ['COMPONENTS', 'EglSupport']),
    # enginio: Not needed in Qt6!
    LibraryMapping('eventdispatchers', 'Qt5', 'Qt::EventDispatcherSupport', extra = ['COMPONENTS', 'EventDispatcherSupport']),
    LibraryMapping('eventdispatcher_support', 'Qt5', 'Qt::EventDispatcherSupport', extra = ['COMPONENTS', 'EventDispatcherSupport']),
    LibraryMapping('extras', 'Qt5', 'Qt::3DExtras', extra = ['COMPONENTS', '3DExtras']),
    LibraryMapping('fbconvenience', 'Qt5', 'Qt::FbSupport', extra = ['COMPONENTS', 'FbSupport']),
    LibraryMapping('fb_support', 'Qt5', 'Qt::FbSupport', extra = ['COMPONENTS', 'FbSupport']),
    LibraryMapping('fontdatabase_support', 'Qt5', 'Qt::FontDatabaseSupport', extra = ['COMPONENTS', 'FontDatabaseSupport']),
    LibraryMapping('gamepad', 'Qt5', 'Qt::Gamepad', extra = ['COMPONENTS', 'Gamepad']),
    LibraryMapping('global', 'Qt5', 'Qt::Core', extra = ['COMPONENTS', 'Core']),  # manually added special case
    LibraryMapping('glx_support', 'Qt5', 'Qt::GlxSupport', extra = ['COMPONENTS', 'GlxSupport']),
    LibraryMapping('graphics_support', 'Qt5', 'Qt::GraphicsSupport', extra = ['COMPONENTS', 'GraphicsSupport']),
    LibraryMapping('gsttools', 'Qt5', 'Qt::MultimediaGstTools', extra = ['COMPONENTS', 'MultimediaGstTools']),
    LibraryMapping('gui', 'Qt5', 'Qt::Gui', extra = ['COMPONENTS', 'Gui']),
    LibraryMapping('help', 'Qt5', 'Qt::Help', extra = ['COMPONENTS', 'Help']),
    LibraryMapping('hunspellinputmethod', 'Qt5', 'Qt::HunspellInputMethod', extra = ['COMPONENTS', 'HunspellInputMethod']),
    LibraryMapping('input', 'Qt5', 'Qt::InputSupport', extra = ['COMPONENTS', 'InputSupport']),
    LibraryMapping('input_support', 'Qt5', 'Qt::InputSupport', extra = ['COMPONENTS', 'InputSupport']),
    LibraryMapping('installer-lib', 'Qt5', 'Qt::AppManInstaller', extra = ['COMPONENTS', 'AppManInstaller']),
    LibraryMapping('kmsconvenience', 'Qt5', 'Qt::KmsSupport', extra = ['COMPONENTS', 'KmsSupport']),
    LibraryMapping('kms_support', 'Qt5', 'Qt::KmsSupport', extra = ['COMPONENTS', 'KmsSupport']),
    LibraryMapping('launcher-lib', 'Qt5', 'Qt::AppManLauncher', extra = ['COMPONENTS', 'AppManLauncher']),
    LibraryMapping('lib', 'Qt5', 'Qt::Designer', extra = ['COMPONENTS', 'Designer']),
    LibraryMapping('linuxaccessibility_support', 'Qt5', 'Qt::LinuxAccessibilitySupport', extra = ['COMPONENTS', 'LinuxAccessibilitySupport']),
    LibraryMapping('location', 'Qt5', 'Qt::Location', extra = ['COMPONENTS', 'Location']),
    LibraryMapping('logic', 'Qt5', 'Qt::3DLogic', extra = ['COMPONENTS', '3DLogic']),
    LibraryMapping('macextras', 'Qt5', 'Qt::MacExtras', extra = ['COMPONENTS', 'MacExtras']),
    LibraryMapping('main-lib', 'Qt5', 'Qt::AppManMain', extra = ['COMPONENTS', 'AppManMain']),
    LibraryMapping('manager-lib', 'Qt5', 'Qt::AppManManager', extra = ['COMPONENTS', 'AppManManager']),
    LibraryMapping('monitor-lib', 'Qt5', 'Qt::AppManMonitor', extra = ['COMPONENTS', 'AppManMonitor']),
    LibraryMapping('multimedia', 'Qt5', 'Qt::Multimedia', extra = ['COMPONENTS', 'Multimedia']),
    LibraryMapping('multimediawidgets', 'Qt5', 'Qt::MultimediaWidgets', extra = ['COMPONENTS', 'MultimediaWidgets']),
    LibraryMapping('network', 'Qt5', 'Qt::Network', extra = ['COMPONENTS', 'Network']),
    LibraryMapping('nfc', 'Qt5', 'Qt::Nfc', extra = ['COMPONENTS', 'Nfc']),
    LibraryMapping('oauth', 'Qt5', 'Qt::NetworkAuth', extra = ['COMPONENTS', 'NetworkAuth']),
    LibraryMapping('openglextensions', 'Qt5', 'Qt::OpenGLExtensions', extra = ['COMPONENTS', 'OpenGLExtensions']),
    LibraryMapping('opengl', 'Qt5', 'Qt::OpenGL', extra = ['COMPONENTS', 'OpenGL']),
    LibraryMapping('package-lib', 'Qt5', 'Qt::AppManPackage', extra = ['COMPONENTS', 'AppManPackage']),
    LibraryMapping('packetprotocol', 'Qt5', 'Qt::PacketProtocol', extra = ['COMPONENTS', 'PacketProtocol']),
    LibraryMapping('particles', 'Qt5', 'Qt::QuickParticles', extra = ['COMPONENTS', 'QuickParticles']),
    LibraryMapping('platformcompositor', 'Qt5', 'Qt::PlatformCompositorSupport', extra = ['COMPONENTS', 'PlatformCompositorSupport']),
    LibraryMapping('platformcompositor_support', 'Qt5', 'Qt::PlatformCompositorSupport', extra = ['COMPONENTS', 'PlatformCompositorSupport']),
    LibraryMapping('plugin-interfaces', 'Qt5', 'Qt::AppManPluginInterfaces', extra = ['COMPONENTS', 'AppManPluginInterfaces']),
    LibraryMapping('positioning', 'Qt5', 'Qt::Positioning', extra = ['COMPONENTS', 'Positioning']),
    LibraryMapping('positioningquick', 'Qt5', 'Qt::PositioningQuick', extra = ['COMPONENTS', 'PositioningQuick']),
    LibraryMapping('printsupport', 'Qt5', 'Qt::PrintSupport', extra = ['COMPONENTS', 'PrintSupport']),
    LibraryMapping('purchasing', 'Qt5', 'Qt::Purchasing', extra = ['COMPONENTS', 'Purchasing']),
    LibraryMapping('qmldebug', 'Qt5', 'Qt::QmlDebug', extra = ['COMPONENTS', 'QmlDebug']),
    LibraryMapping('qmldevtools', 'Qt5', 'Qt::QmlDevTools', extra = ['COMPONENTS', 'QmlDevTools']),
    LibraryMapping('qml', 'Qt5', 'Qt::Qml', extra = ['COMPONENTS', 'Qml']),
    LibraryMapping('qmltest', 'Qt5', 'Qt::QuickTest', extra = ['COMPONENTS', 'QuickTest']),
    LibraryMapping('qtmultimediaquicktools', 'Qt5', 'Qt::MultimediaQuick', extra = ['COMPONENTS', 'MultimediaQuick']),
    LibraryMapping('quick3danimation', 'Qt5', 'Qt::3DQuickAnimation', extra = ['COMPONENTS', '3DQuickAnimation']),
    LibraryMapping('quick3dextras', 'Qt5', 'Qt::3DQuickExtras', extra = ['COMPONENTS', '3DQuickExtras']),
    LibraryMapping('quick3dinput', 'Qt5', 'Qt::3DQuickInput', extra = ['COMPONENTS', '3DQuickInput']),
    LibraryMapping('quick3d', 'Qt5', 'Qt::3DQuick', extra = ['COMPONENTS', '3DQuick']),
    LibraryMapping('quick3drender', 'Qt5', 'Qt::3DQuickRender', extra = ['COMPONENTS', '3DQuickRender']),
    LibraryMapping('quick3dscene2d', 'Qt5', 'Qt::3DQuickScene2D', extra = ['COMPONENTS', '3DQuickScene2D']),
    LibraryMapping('quickcontrols2', 'Qt5', 'Qt::QuickControls2', extra = ['COMPONENTS', 'QuickControls2']),
    LibraryMapping('quick', 'Qt5', 'Qt::Quick', extra = ['COMPONENTS', 'Quick']),
    LibraryMapping('quickshapes', 'Qt5', 'Qt::QuickShapes', extra = ['COMPONENTS', 'QuickShapes']),
    LibraryMapping('quicktemplates2', 'Qt5', 'Qt::QuickTemplates2', extra = ['COMPONENTS', 'QuickTemplates2']),
    LibraryMapping('quickwidgets', 'Qt5', 'Qt::QuickWidgets', extra = ['COMPONENTS', 'QuickWidgets']),
    LibraryMapping('render', 'Qt5', 'Qt::3DRender', extra = ['COMPONENTS', '3DRender']),
    LibraryMapping('script', 'Qt5', 'Qt::Script', extra = ['COMPONENTS', 'Script']),
    LibraryMapping('scripttools', 'Qt5', 'Qt::ScriptTools', extra = ['COMPONENTS', 'ScriptTools']),
    LibraryMapping('sensors', 'Qt5', 'Qt::Sensors', extra = ['COMPONENTS', 'Sensors']),
    LibraryMapping('serialport', 'Qt5', 'Qt::SerialPort', extra = ['COMPONENTS', 'SerialPort']),
    LibraryMapping('services', 'Qt5', 'Qt::ServiceSupport', extra = ['COMPONENTS', 'ServiceSupport']),
    LibraryMapping('service_support', 'Qt5', 'Qt::ServiceSupport', extra = ['COMPONENTS', 'ServiceSupport']),
    LibraryMapping('sql', 'Qt5', 'Qt::Sql', extra = ['COMPONENTS', 'Sql']),
    LibraryMapping('svg', 'Qt5', 'Qt::Svg', extra = ['COMPONENTS', 'Svg']),
    LibraryMapping('testlib', 'Qt5', 'Qt::Test', extra = ['COMPONENTS', 'Test']),
    LibraryMapping('theme_support', 'Qt5', 'Qt::ThemeSupport', extra = ['COMPONENTS', 'ThemeSupport']),
    LibraryMapping('tts', 'Qt5', 'Qt::TextToSpeech', extra = ['COMPONENTS', 'TextToSpeech']),
    LibraryMapping('uiplugin', 'Qt5', 'Qt::UiPlugin', extra = ['COMPONENTS', 'UiPlugin']),
    LibraryMapping('uitools', 'Qt5', 'Qt::UiTools', extra = ['COMPONENTS', 'UiTools']),
    LibraryMapping('virtualkeyboard', 'Qt5', 'Qt::VirtualKeyboard', extra = ['COMPONENTS', 'VirtualKeyboard']),
    LibraryMapping('vulkan_support', 'Qt5', 'Qt::VulkanSupport', extra = ['COMPONENTS', 'VulkanSupport']),
    LibraryMapping('webchannel', 'Qt5', 'Qt::WebChannel', extra = ['COMPONENTS', 'WebChannel']),
    LibraryMapping('webengine', 'Qt5', 'Qt::WebEngine', extra = ['COMPONENTS', 'WebEngine']),
    LibraryMapping('webenginewidgets', 'Qt5', 'Qt::WebEngineWidgets', extra = ['COMPONENTS', 'WebEngineWidgets']),
    LibraryMapping('websockets', 'Qt5', 'Qt::WebSockets', extra = ['COMPONENTS', 'WebSockets']),
    LibraryMapping('webview', 'Qt5', 'Qt::WebView', extra = ['COMPONENTS', 'WebView']),
    LibraryMapping('widgets', 'Qt5', 'Qt::Widgets', extra = ['COMPONENTS', 'Widgets']),
    LibraryMapping('window-lib', 'Qt5', 'Qt::AppManWindow', extra = ['COMPONENTS', 'AppManWindow']),
    LibraryMapping('windowsuiautomation_support', 'Qt5', 'Qt::WindowsUIAutomationSupport', extra = ['COMPONENTS', 'WindowsUIAutomationSupport']),
    LibraryMapping('winextras', 'Qt5', 'Qt::WinExtras', extra = ['COMPONENTS', 'WinExtras']),
    LibraryMapping('x11extras', 'Qt5', 'Qt::X11Extras', extra = ['COMPONENTS', 'X11Extras']),
    LibraryMapping('xkbcommon_support', 'Qt5', 'Qt::XkbCommonSupport', extra = ['COMPONENTS', 'XkbCommonSupport']),
    LibraryMapping('xmlpatterns', 'Qt5', 'Qt::XmlPatterns', extra = ['COMPONENTS', 'XmlPatterns']),
    LibraryMapping('xml', 'Qt5', 'Qt::Xml', extra = ['COMPONENTS', 'Xml']),
    # qtzlib: No longer supported.
]

_library_map = [
    # 3rd party:
    LibraryMapping('atspi', 'ATSPI2', 'PkgConfig::ATSPI2'),
    LibraryMapping('corewlan', None, None),
    LibraryMapping('cups', 'Cups', 'Cups::Cups'),
    LibraryMapping('dbus', 'DBus1', 'dbus-1'),
    LibraryMapping('doubleconversion', None, None),
    LibraryMapping('drm', 'Libdrm', 'Libdrm::Libdrm'),
    LibraryMapping('egl', 'EGL', 'EGL::EGL'),
    LibraryMapping('fontconfig', 'Fontconfig', 'Fontconfig::Fontconfig', resultVariable="FONTCONFIG"),
    LibraryMapping('freetype', 'Freetype', 'Freetype::Freetype', extra=['REQUIRED']),
    LibraryMapping('gbm', 'gbm', 'gbm::gbm'),
    LibraryMapping('glib', 'GLIB2', 'GLIB2::GLIB2'),
    LibraryMapping('gnu_iconv', None, None),
    LibraryMapping('gtk3', 'GTK3', 'PkgConfig::GTK3'),
    LibraryMapping('harfbuzz', 'harfbuzz', 'harfbuzz::harfbuzz'),
    LibraryMapping('host_dbus', None, None),
    LibraryMapping('icu', 'ICU', 'ICU::i18n ICU::uc ICU::data', extra=['COMPONENTS', 'i18n', 'uc', 'data']),
    LibraryMapping('journald', 'Libsystemd', 'PkgConfig::Libsystemd'),
    LibraryMapping('jpeg', 'JPEG', 'JPEG::JPEG'), # see also libjpeg
    LibraryMapping('libatomic', 'Atomic', 'Atomic'),
    LibraryMapping('libdl', None, '${CMAKE_DL_LIBS}'),
    LibraryMapping('libinput', 'Libinput', 'Libinput::Libinput'),
    LibraryMapping('libjpeg', 'JPEG', 'JPEG::JPEG'), # see also jpeg
    LibraryMapping('libpng', 'PNG', 'PNG::PNG'),
    LibraryMapping('libproxy', 'Libproxy', 'PkgConfig::Libproxy'),
    LibraryMapping('librt', 'WrapRt','WrapRt'),
    LibraryMapping('libudev', 'Libudev', 'PkgConfig::Libudev'),
    LibraryMapping('lttng-ust', 'LTTngUST', 'LTTng::UST', resultVariable='LTTNGUST'),
    LibraryMapping('mtdev', 'Mtdev', 'PkgConfig::Mtdev'),
    LibraryMapping('odbc', 'ODBC', 'ODBC::ODBC'),
    LibraryMapping('opengl_es2', 'GLESv2', 'GLESv2::GLESv2'),
    LibraryMapping('opengl', 'OpenGL', 'OpenGL::GL', resultVariable='OpenGL_OpenGL'),
    LibraryMapping('openssl_headers', 'OpenSSL', 'OpenSSL::SSL_nolink', resultVariable='OPENSSL_INCLUDE_DIR', appendFoundSuffix=False),
    LibraryMapping('openssl', 'OpenSSL', 'OpenSSL::SSL'),
    LibraryMapping('pcre2', 'PCRE2', 'PCRE2', extra = ['REQUIRED']),
    LibraryMapping('posix_iconv', None, None),
    LibraryMapping('pps', 'PPS', 'PPS::PPS'),
    LibraryMapping('psql', 'PostgreSQL', 'PostgreSQL::PostgreSQL'),
    LibraryMapping('slog2', 'Slog2', 'Slog2::Slog2'),
    LibraryMapping('sqlite2', None, None), # No more sqlite2 support in Qt6!
    LibraryMapping('sqlite3', 'SQLite3', 'SQLite::SQLite3'),
    LibraryMapping('sun_iconv', None, None),
    LibraryMapping('tslib', 'Tslib', 'PkgConfig::Tslib'),
    LibraryMapping('udev', 'Libudev', 'PkgConfig::Libudev'),
    LibraryMapping('udev', 'Libudev', 'PkgConfig::Libudev'), # see also libudev!
    LibraryMapping('vulkan', 'Vulkan', 'Vulkan::Vulkan'),
    LibraryMapping('wayland_server', 'Wayland', 'Wayland::Server'),
    LibraryMapping('x11sm', 'X11', '${X11_SM_LIB} ${X11_ICE_LIB}', resultVariable="X11_SM"),
    LibraryMapping('xcb_glx', 'XCB', 'XCB::GLX', resultVariable='XCB_GLX'),
    LibraryMapping('xcb_render', 'XCB', 'XCB::RENDER', resultVariable='XCB_RENDER'),
    LibraryMapping('xcb', 'XCB', 'XCB::XCB', extra = ['1.9']),
    LibraryMapping('xcb_glx', 'XCB', 'XCB::GLX', extra = ['COMPONENTS', 'GLX'], resultVariable='XCB_GLX'),
    LibraryMapping('xcb_icccm', 'XCB', 'XCB::ICCCM', extra = ['COMPONENTS', 'ICCCM'], resultVariable='XCB_ICCCM'),
    LibraryMapping('xcb_image', 'XCB', 'XCB::IMAGE', extra = ['COMPONENTS', 'IMAGE'], resultVariable='XCB_IMAGE'),
    LibraryMapping('xcb_keysyms', 'XCB', 'XCB::KEYSYMS', extra = ['COMPONENTS', 'KEYSYMS'], resultVariable='XCB_KEYSYMS'),
    LibraryMapping('xcb_randr', 'XCB', 'XCB::RANDR', extra = ['COMPONENTS', 'RANDR'], resultVariable='XCB_RANDR'),
    LibraryMapping('xcb_render', 'XCB', 'XCB::RENDER', extra = ['COMPONENTS', 'RENDER'], resultVariable='XCB_RENDER'),
    LibraryMapping('xcb_renderutil', 'XCB', 'XCB::RENDERUTIL', extra = ['COMPONENTS', 'RENDERUTIL'], resultVariable='XCB_RENDERUTIL'),
    LibraryMapping('xcb_shape', 'XCB', 'XCB::SHAPE', extra = ['COMPONENTS', 'SHAPE'], resultVariable='XCB_SHAPE'),
    LibraryMapping('xcb_shm', 'XCB', 'XCB::SHM', extra = ['COMPONENTS', 'SHM'], resultVariable='XCB_SHM'),
    LibraryMapping('xcb_sync', 'XCB', 'XCB::SYNC', extra = ['COMPONENTS', 'SYNC'], resultVariable='XCB_SYNC'),
    LibraryMapping('xcb_xfixes', 'XCB', 'XCB::XFIXES', extra = ['COMPONENTS', 'XFIXES'], resultVariable='XCB_XFIXES'),
    LibraryMapping('xcb_xinerama', 'XCB', 'XCB::XINERAMA', extra = ['COMPONENTS', 'XINERAMA'], resultVariable='XCB_XINERAMA'),
    LibraryMapping('xcb_xinput', 'XCB', 'XCB::XINPUT', extra = ['COMPONENTS', 'XINPUT'], resultVariable='XCB_XINPUT'),
    LibraryMapping('xcb_xkb', 'XCB', 'XCB::XKB', extra = ['COMPONENTS', 'XKB'], resultVariable='XCB_XKB'),
    LibraryMapping('xcb_xlib', 'X11_XCB', 'X11::XCB'),
    LibraryMapping('xkbcommon_evdev', 'XKB', 'XKB::XKB', extra = ['0.4.1']), # see also xkbcommon
    LibraryMapping('xkbcommon_x11', 'XKB', 'XKB::XKB', extra = ['0.4.1']), # see also xkbcommon
    LibraryMapping('xkbcommon', 'XKB', 'XKB::XKB', extra = ['0.4.1']),
    LibraryMapping('xlib', 'X11', 'X11::XCB'), # FIXME: Is this correct?
    LibraryMapping('xrender', 'XRender', 'PkgConfig::xrender'),
    LibraryMapping('zlib', 'ZLIB', 'ZLIB::ZLIB', extra=['REQUIRED']),
    LibraryMapping('zstd', 'ZSTD', 'ZSTD::ZSTD'),
]


def find_3rd_party_library_mapping(soName: str) -> typing.Optional[LibraryMapping]:
    for i in _library_map:
        if i.soName == soName:
            return i
    return None


def find_qt_library_mapping(soName: str) -> typing.Optional[LibraryMapping]:
    for i in _qt_library_map:
        if i.soName == soName:
            return i
    return None


def featureName(input: str) -> str:
    return re.sub(r'[^a-zA-Z0-9_]', '_', input)


def map_qt_library(lib: str) -> str:
    private = False
    if lib.endswith('-private'):
        private = True
        lib = lib[:-8]
    mapped = find_qt_library_mapping(lib)
    qt_name = lib
    if mapped:
        assert mapped.targetName # Qt libs must have a target name set
        qt_name = mapped.targetName
    if private:
        qt_name += 'Private'
    return qt_name


platform_mapping = {
    'win32': 'WIN32',
    'unix': 'UNIX',
    'darwin': 'APPLE',
    'linux': 'LINUX',
    'integrity': 'INTEGRITY',
    'qnx': 'QNX',
    'vxworks': 'VXWORKS',
    'hpux': 'HPUX',
    'nacl': 'NACL',
    'android': 'ANDROID',
    'android-embedded': 'ANDROID_EMBEDDED',
    'uikit': 'APPLE_UIKIT',
    'tvos': 'APPLE_TVOS',
    'watchos': 'APPLE_WATCHOS',
    'winrt': 'WINRT',
    'wasm': 'WASM',
    'msvc': 'MSVC',
    'clang': 'CLANG',
    'gcc': 'GCC',
    'icc': 'ICC',
    'intel_icc': 'ICC',
    'osx': 'APPLE_OSX',
    'ios': 'APPLE_IOS',
    'freebsd': 'FREEBSD',
    'openbsd': 'OPENBSD',
    'netbsd': 'NETBSD',
    'haiku': 'HAIKU',
    'netbsd': 'NETBSD',
    'mac': 'APPLE_OSX',
    'macx': 'APPLE_OSX',
    'macos': 'APPLE_OSX',
    'macx-icc': '(APPLE_OSX AND ICC)',
}


def map_platform(platform: str) -> str:
    """ Return the qmake platform as cmake platform or the unchanged string. """
    return platform_mapping.get(platform, platform)


def is_known_3rd_party_library(lib: str) -> bool:
    if lib.endswith('/nolink') or lib.endswith('_nolink'):
        lib = lib[:-7]
    mapping = find_3rd_party_library_mapping(lib)

    return mapping is not None and mapping.targetName is not None


def map_3rd_party_library(lib: str) -> str:
    libpostfix = ''
    if lib.endswith('/nolink'):
        lib = lib[:-7]
        libpostfix = '_nolink'
    mapping = find_3rd_party_library_mapping(lib)
    if not mapping or not mapping.targetName:
        return lib
    return mapping.targetName + libpostfix
