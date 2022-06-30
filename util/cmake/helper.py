#############################################################################
##
## Copyright (C) 2021 The Qt Company Ltd.
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
    def __init__(
        self,
        soName: str,
        packageName: typing.Optional[str],
        targetName: typing.Optional[str],
        *,
        resultVariable: typing.Optional[str] = None,
        extra: typing.List[str] = [],
        appendFoundSuffix: bool = True,
        emit_if: str = "",
        is_bundled_with_qt: bool = False,
        test_library_overwrite: str = "",
        run_library_test: bool = False,
        no_link_so_name: str = "",
    ) -> None:
        self.soName = soName
        self.packageName = packageName
        self.resultVariable = resultVariable
        self.appendFoundSuffix = appendFoundSuffix
        # Allows passing addiitonal arguments to the generated find_package call.
        self.extra = extra
        self.targetName = targetName

        # True if qt bundles the library sources as part of Qt.
        self.is_bundled_with_qt = is_bundled_with_qt

        # if emit_if is non-empty, the generated find_package call
        # for a library will be surrounded by this condition.
        self.emit_if = emit_if

        # Allow overwriting library name when used with tests. E.g.: _nolink
        # targets do not exist when used during compile tests
        self.test_library_overwrite = test_library_overwrite

        # Run the library compile test of configure.json
        self.run_library_test = run_library_test

        # The custom nolink library mapping associated with this one.
        self.no_link_so_name = no_link_so_name

    def is_qt(self) -> bool:
        return self.packageName == "Qt" or self.packageName == "Qt5" or self.packageName == "Qt6"


_qt_library_map = [
    # Qt:
    LibraryMapping(
        "androidextras", "Qt6", "Qt::AndroidExtras", extra=["COMPONENTS", "AndroidExtras"]
    ),
    LibraryMapping("3danimation", "Qt6", "Qt::3DAnimation", extra=["COMPONENTS", "3DAnimation"]),
    LibraryMapping("3dcore", "Qt6", "Qt::3DCore", extra=["COMPONENTS", "3DCore"]),
    LibraryMapping("3dcoretest", "Qt6", "Qt::3DCoreTest", extra=["COMPONENTS", "3DCoreTest"]),
    LibraryMapping("3dextras", "Qt6", "Qt::3DExtras", extra=["COMPONENTS", "3DExtras"]),
    LibraryMapping("3dinput", "Qt6", "Qt::3DInput", extra=["COMPONENTS", "3DInput"]),
    LibraryMapping("3dlogic", "Qt6", "Qt::3DLogic", extra=["COMPONENTS", "3DLogic"]),
    LibraryMapping("3dquick", "Qt6", "Qt::3DQuick", extra=["COMPONENTS", "3DQuick"]),
    LibraryMapping(
        "3dquickextras", "Qt6", "Qt::3DQuickExtras", extra=["COMPONENTS", "3DQuickExtras"]
    ),
    LibraryMapping("3dquickinput", "Qt6", "Qt::3DQuickInput", extra=["COMPONENTS", "3DQuickInput"]),
    LibraryMapping(
        "3dquickrender", "Qt6", "Qt::3DQuickRender", extra=["COMPONENTS", "3DQuickRender"]
    ),
    LibraryMapping("3drender", "Qt6", "Qt::3DRender", extra=["COMPONENTS", "3DRender"]),
    LibraryMapping(
        "application-lib", "Qt6", "Qt::AppManApplication", extra=["COMPONENTS", "AppManApplication"]
    ),
    LibraryMapping("axbase", "Qt6", "Qt::AxBasePrivate", extra=["COMPONENTS", "AxBasePrivate"]),
    LibraryMapping("axcontainer", "Qt6", "Qt::AxContainer", extra=["COMPONENTS", "AxContainer"]),
    LibraryMapping("axserver", "Qt6", "Qt::AxServer", extra=["COMPONENTS", "AxServer"]),
    LibraryMapping("bluetooth", "Qt6", "Qt::Bluetooth", extra=["COMPONENTS", "Bluetooth"]),
    LibraryMapping("bootstrap", "Qt6", "Qt::Bootstrap", extra=["COMPONENTS", "Bootstrap"]),
    # bootstrap-dbus: Not needed in Qt6!
    LibraryMapping("client", "Qt6", "Qt::WaylandClient", extra=["COMPONENTS", "WaylandClient"]),
    LibraryMapping("coap", "Qt6", "Qt::Coap", extra=["COMPONENTS", "Coap"]),
    LibraryMapping("common-lib", "Qt6", "Qt::AppManCommon", extra=["COMPONENTS", "AppManCommon"]),
    LibraryMapping(
        "compositor", "Qt6", "Qt::WaylandCompositor", extra=["COMPONENTS", "WaylandCompositor"]
    ),
    LibraryMapping("concurrent", "Qt6", "Qt::Concurrent", extra=["COMPONENTS", "Concurrent"]),
    LibraryMapping("container", "Qt6", "Qt::AxContainer", extra=["COMPONENTS", "AxContainer"]),
    LibraryMapping("control", "Qt6", "Qt::AxServer", extra=["COMPONENTS", "AxServer"]),
    LibraryMapping(
        "core_headers", "Qt6", "Qt::WebEngineCore", extra=["COMPONENTS", "WebEngineCore"]
    ),
    LibraryMapping("core", "Qt6", "Qt::Core", extra=["COMPONENTS", "Core"]),
    LibraryMapping("crypto-lib", "Qt6", "Qt::AppManCrypto", extra=["COMPONENTS", "AppManCrypto"]),
    LibraryMapping("dbus", "Qt6", "Qt::DBus", extra=["COMPONENTS", "DBus"]),
    LibraryMapping("designer", "Qt6", "Qt::Designer", extra=["COMPONENTS", "Designer"]),
    LibraryMapping(
        "designercomponents",
        "Qt6",
        "Qt::DesignerComponentsPrivate",
        extra=["COMPONENTS", "DesignerComponentsPrivate"],
    ),
    LibraryMapping(
        "devicediscovery",
        "Qt6",
        "Qt::DeviceDiscoverySupportPrivate",
        extra=["COMPONENTS", "DeviceDiscoverySupportPrivate"],
    ),
    LibraryMapping(
        "devicediscovery_support",
        "Qt6",
        "Qt::DeviceDiscoverySupportPrivate",
        extra=["COMPONENTS", "DeviceDiscoverySupportPrivate"],
    ),
    LibraryMapping("edid", "Qt6", "Qt::EdidSupport", extra=["COMPONENTS", "EdidSupport"]),
    LibraryMapping("edid_support", "Qt6", "Qt::EdidSupport", extra=["COMPONENTS", "EdidSupport"]),
    LibraryMapping("eglconvenience", "Qt6", "Qt::EglSupport", extra=["COMPONENTS", "EglSupport"]),
    LibraryMapping(
        "eglfsdeviceintegration",
        "Qt6",
        "Qt::EglFSDeviceIntegrationPrivate",
        extra=["COMPONENTS", "EglFSDeviceIntegrationPrivate"],
    ),
    LibraryMapping(
        "eglfs_kms_support",
        "Qt6",
        "Qt::EglFsKmsSupportPrivate",
        extra=["COMPONENTS", "EglFsKmsSupportPrivate"],
    ),
    LibraryMapping(
        "eglfs_kms_gbm_support",
        "Qt6",
        "Qt::EglFsKmsGbmSupportPrivate",
        extra=["COMPONENTS", "EglFsKmsGbmSupportPrivate"],
    ),
    LibraryMapping("egl_support", "Qt6", "Qt::EglSupport", extra=["COMPONENTS", "EglSupport"]),
    # enginio: Not needed in Qt6!
    LibraryMapping(
        "eventdispatchers",
        "Qt6",
        "Qt::EventDispatcherSupport",
        extra=["COMPONENTS", "EventDispatcherSupport"],
    ),
    LibraryMapping(
        "eventdispatcher_support",
        "Qt6",
        "Qt::EventDispatcherSupport",
        extra=["COMPONENTS", "EventDispatcherSupport"],
    ),
    LibraryMapping(
        "fbconvenience", "Qt6", "Qt::FbSupportPrivate", extra=["COMPONENTS", "FbSupportPrivate"]
    ),
    LibraryMapping(
        "fb_support", "Qt6", "Qt::FbSupportPrivate", extra=["COMPONENTS", "FbSupportPrivate"]
    ),
    LibraryMapping(
        "fontdatabase_support",
        "Qt6",
        "Qt::FontDatabaseSupport",
        extra=["COMPONENTS", "FontDatabaseSupport"],
    ),
    LibraryMapping("gamepad", "Qt6", "Qt::Gamepad", extra=["COMPONENTS", "Gamepad"]),
    LibraryMapping("geniviextras", "Qt6", "Qt::GeniviExtras", extra=["COMPONENTS", "GeniviExtras"]),
    LibraryMapping(
        "global", "Qt6", "Qt::Core", extra=["COMPONENTS", "Core"]
    ),  # manually added special case
    LibraryMapping("glx_support", "Qt6", "Qt::GlxSupport", extra=["COMPONENTS", "GlxSupport"]),
    LibraryMapping(
        "gsttools", "Qt6", "Qt::MultimediaGstTools", extra=["COMPONENTS", "MultimediaGstTools"]
    ),
    LibraryMapping("gui", "Qt6", "Qt::Gui", extra=["COMPONENTS", "Gui"]),
    LibraryMapping("help", "Qt6", "Qt::Help", extra=["COMPONENTS", "Help"]),
    LibraryMapping(
        "hunspellinputmethod",
        "Qt6",
        "Qt::HunspellInputMethodPrivate",
        extra=["COMPONENTS", "HunspellInputMethodPrivate"],
    ),
    LibraryMapping(
        "input", "Qt6", "Qt::InputSupportPrivate", extra=["COMPONENTS", "InputSupportPrivate"]
    ),
    LibraryMapping(
        "input_support",
        "Qt6",
        "Qt::InputSupportPrivate",
        extra=["COMPONENTS", "InputSupportPrivate"],
    ),
    LibraryMapping(
        "installer-lib", "Qt6", "Qt::AppManInstaller", extra=["COMPONENTS", "AppManInstaller"]
    ),
    LibraryMapping("ivi", "Qt6", "Qt::Ivi", extra=["COMPONENTS", "Ivi"]),
    LibraryMapping("ivicore", "Qt6", "Qt::IviCore", extra=["COMPONENTS", "IviCore"]),
    LibraryMapping("ivimedia", "Qt6", "Qt::IviMedia", extra=["COMPONENTS", "IviMedia"]),
    LibraryMapping("knx", "Qt6", "Qt::Knx", extra=["COMPONENTS", "Knx"]),
    LibraryMapping(
        "kmsconvenience", "Qt6", "Qt::KmsSupportPrivate", extra=["COMPONENTS", "KmsSupportPrivate"]
    ),
    LibraryMapping(
        "kms_support", "Qt6", "Qt::KmsSupportPrivate", extra=["COMPONENTS", "KmsSupportPrivate"]
    ),
    LibraryMapping(
        "launcher-lib", "Qt6", "Qt::AppManLauncher", extra=["COMPONENTS", "AppManLauncher"]
    ),
    LibraryMapping("lib", "Qt6", "Qt::Designer", extra=["COMPONENTS", "Designer"]),
    LibraryMapping(
        "linuxaccessibility_support",
        "Qt6",
        "Qt::LinuxAccessibilitySupport",
        extra=["COMPONENTS", "LinuxAccessibilitySupport"],
    ),
    LibraryMapping("location", "Qt6", "Qt::Location", extra=["COMPONENTS", "Location"]),
    LibraryMapping("macextras", "Qt6", "Qt::MacExtras", extra=["COMPONENTS", "MacExtras"]),
    LibraryMapping("main-lib", "Qt6", "Qt::AppManMain", extra=["COMPONENTS", "AppManMain"]),
    LibraryMapping(
        "manager-lib", "Qt6", "Qt::AppManManager", extra=["COMPONENTS", "AppManManager"]
    ),
    LibraryMapping(
        "monitor-lib", "Qt6", "Qt::AppManMonitor", extra=["COMPONENTS", "AppManMonitor"]
    ),
    LibraryMapping("mqtt", "Qt6", "Qt::Mqtt", extra=["COMPONENTS", "Mqtt"]),
    LibraryMapping("multimedia", "Qt6", "Qt::Multimedia", extra=["COMPONENTS", "Multimedia"]),
    LibraryMapping(
        "multimediawidgets",
        "Qt6",
        "Qt::MultimediaWidgets",
        extra=["COMPONENTS", "MultimediaWidgets"],
    ),
    LibraryMapping("network", "Qt6", "Qt::Network", extra=["COMPONENTS", "Network"]),
    LibraryMapping("networkauth", "Qt6", "Qt::NetworkAuth", extra=["COMPONENTS", "NetworkAuth"]),
    LibraryMapping("nfc", "Qt6", "Qt::Nfc", extra=["COMPONENTS", "Nfc"]),
    LibraryMapping("oauth", "Qt6", "Qt::NetworkAuth", extra=["COMPONENTS", "NetworkAuth"]),
    LibraryMapping("opcua", "Qt6", "Qt::OpcUa", extra=["COMPONENTS", "OpcUa"]),
    LibraryMapping(
        "opcua_private", "Qt6", "Qt::OpcUaPrivate", extra=["COMPONENTS", "OpcUaPrivate"]
    ),
    LibraryMapping("opengl", "Qt6", "Qt::OpenGL", extra=["COMPONENTS", "OpenGL"]),
    LibraryMapping(
        "openglwidgets", "Qt6", "Qt::OpenGLWidgets", extra=["COMPONENTS", "OpenGLWidgets"]
    ),
    LibraryMapping(
        "package-lib", "Qt6", "Qt::AppManPackage", extra=["COMPONENTS", "AppManPackage"]
    ),
    LibraryMapping(
        "packetprotocol",
        "Qt6",
        "Qt::PacketProtocolPrivate",
        extra=["COMPONENTS", "PacketProtocolPrivate"],
    ),
    LibraryMapping(
        "particles",
        "Qt6",
        "Qt::QuickParticlesPrivate",
        extra=["COMPONENTS", "QuickParticlesPrivate"],
    ),
    LibraryMapping(
        "plugin-interfaces",
        "Qt6",
        "Qt::AppManPluginInterfaces",
        extra=["COMPONENTS", "AppManPluginInterfaces"],
    ),
    LibraryMapping("positioning", "Qt6", "Qt::Positioning", extra=["COMPONENTS", "Positioning"]),
    LibraryMapping(
        "positioningquick", "Qt6", "Qt::PositioningQuick", extra=["COMPONENTS", "PositioningQuick"]
    ),
    LibraryMapping("printsupport", "Qt6", "Qt::PrintSupport", extra=["COMPONENTS", "PrintSupport"]),
    LibraryMapping("purchasing", "Qt6", "Qt::Purchasing", extra=["COMPONENTS", "Purchasing"]),
    LibraryMapping(
        "qmldebug", "Qt6", "Qt::QmlDebugPrivate", extra=["COMPONENTS", "QmlDebugPrivate"]
    ),
    LibraryMapping(
        "qmldevtools", "Qt6", "Qt::QmlDevToolsPrivate", extra=["COMPONENTS", "QmlDevToolsPrivate"]
    ),
    LibraryMapping(
        "qmlcompiler", "Qt6", "Qt::QmlCompilerPrivate", extra=["COMPONENTS", "QmlCompilerPrivate"]
    ),
    LibraryMapping("qml", "Qt6", "Qt::Qml", extra=["COMPONENTS", "Qml"]),
    LibraryMapping("qmldom", "Qt6", "Qt::QmlDomPrivate", extra=["COMPONENTS", "QmlDomPrivate"]),
    LibraryMapping("qmlmodels", "Qt6", "Qt::QmlModels", extra=["COMPONENTS", "QmlModels"]),
    LibraryMapping("qmltest", "Qt6", "Qt::QuickTest", extra=["COMPONENTS", "QuickTest"]),
    LibraryMapping(
        "qtmultimediaquicktools",
        "Qt6",
        "Qt::MultimediaQuickPrivate",
        extra=["COMPONENTS", "MultimediaQuickPrivate"],
    ),
    LibraryMapping(
        "quick3dassetimport",
        "Qt6",
        "Qt::Quick3DAssetImport",
        extra=["COMPONENTS", "Quick3DAssetImport"],
    ),
    LibraryMapping("core5compat", "Qt6", "Qt::Core5Compat", extra=["COMPONENTS", "Core5Compat"]),
    LibraryMapping("quick3d", "Qt6", "Qt::Quick3D", extra=["COMPONENTS", "Quick3D"]),
    LibraryMapping(
        "quick3drender", "Qt6", "Qt::Quick3DRender", extra=["COMPONENTS", "Quick3DRender"]
    ),
    LibraryMapping(
        "quick3druntimerender",
        "Qt6",
        "Qt::Quick3DRuntimeRender",
        extra=["COMPONENTS", "Quick3DRuntimeRender"],
    ),
    LibraryMapping("quick3dutils", "Qt6", "Qt::Quick3DUtils", extra=["COMPONENTS", "Quick3DUtils"]),
    LibraryMapping(
        "quickcontrols2", "Qt6", "Qt::QuickControls2", extra=["COMPONENTS", "QuickControls2"]
    ),
    LibraryMapping(
        "quickcontrols2impl",
        "Qt6",
        "Qt::QuickControls2Impl",
        extra=["COMPONENTS", "QuickControls2Impl"],
    ),
    LibraryMapping("quick", "Qt6", "Qt::Quick", extra=["COMPONENTS", "Quick"]),
    LibraryMapping(
        "quickshapes", "Qt6", "Qt::QuickShapesPrivate", extra=["COMPONENTS", "QuickShapesPrivate"]
    ),
    LibraryMapping(
        "quicktemplates2", "Qt6", "Qt::QuickTemplates2", extra=["COMPONENTS", "QuickTemplates2"]
    ),
    LibraryMapping("quickwidgets", "Qt6", "Qt::QuickWidgets", extra=["COMPONENTS", "QuickWidgets"]),
    LibraryMapping(
        "remoteobjects", "Qt6", "Qt::RemoteObjects", extra=["COMPONENTS", "RemoteObjects"]
    ),
    LibraryMapping("script", "Qt6", "Qt::Script", extra=["COMPONENTS", "Script"]),
    LibraryMapping("scripttools", "Qt6", "Qt::ScriptTools", extra=["COMPONENTS", "ScriptTools"]),
    LibraryMapping("scxml", "Qt6", "Qt::Scxml", extra=["COMPONENTS", "Scxml"]),
    LibraryMapping("sensors", "Qt6", "Qt::Sensors", extra=["COMPONENTS", "Sensors"]),
    LibraryMapping("serialport", "Qt6", "Qt::SerialPort", extra=["COMPONENTS", "SerialPort"]),
    LibraryMapping("serialbus", "Qt6", "Qt::SerialBus", extra=["COMPONENTS", "SerialBus"]),
    LibraryMapping("services", "Qt6", "Qt::ServiceSupport", extra=["COMPONENTS", "ServiceSupport"]),
    LibraryMapping(
        "service_support", "Qt6", "Qt::ServiceSupport", extra=["COMPONENTS", "ServiceSupport"]
    ),
    LibraryMapping("shadertools", "Qt6", "Qt::ShaderTools", extra=["COMPONENTS", "ShaderTools"]),
    LibraryMapping("statemachine", "Qt6", "Qt::StateMachine", extra=["COMPONENTS", "StateMachine"]),
    LibraryMapping("sql", "Qt6", "Qt::Sql", extra=["COMPONENTS", "Sql"]),
    LibraryMapping("svg", "Qt6", "Qt::Svg", extra=["COMPONENTS", "Svg"]),
    LibraryMapping("svgwidgets", "Qt6", "Qt::SvgWidgets", extra=["COMPONENTS", "SvgWidgets"]),
    LibraryMapping("charts", "Qt6", "Qt::Charts", extra=["COMPONENTS", "Charts"]),
    LibraryMapping("testlib", "Qt6", "Qt::Test", extra=["COMPONENTS", "Test"]),
    LibraryMapping("texttospeech", "Qt6", "Qt::TextToSpeech", extra=["COMPONENTS", "TextToSpeech"]),
    LibraryMapping(
        "theme_support", "Qt6", "Qt::ThemeSupport", extra=["COMPONENTS", "ThemeSupport"]
    ),
    LibraryMapping("tts", "Qt6", "Qt::TextToSpeech", extra=["COMPONENTS", "TextToSpeech"]),
    LibraryMapping("uiplugin", "Qt6", "Qt::UiPlugin", extra=["COMPONENTS", "UiPlugin"]),
    LibraryMapping("uitools", "Qt6", "Qt::UiTools", extra=["COMPONENTS", "UiTools"]),
    LibraryMapping(
        "virtualkeyboard", "Qt6", "Qt::VirtualKeyboard", extra=["COMPONENTS", "VirtualKeyboard"]
    ),
    LibraryMapping(
        "waylandclient", "Qt6", "Qt::WaylandClient", extra=["COMPONENTS", "WaylandClient"]
    ),
    LibraryMapping(
        "waylandcompositor",
        "Qt6",
        "Qt::WaylandCompositor",
        extra=["COMPONENTS", "WaylandCompositor"],
    ),
    LibraryMapping("webchannel", "Qt6", "Qt::WebChannel", extra=["COMPONENTS", "WebChannel"]),
    LibraryMapping("webengine", "Qt6", "Qt::WebEngine", extra=["COMPONENTS", "WebEngine"]),
    LibraryMapping(
        "webenginewidgets", "Qt6", "Qt::WebEngineWidgets", extra=["COMPONENTS", "WebEngineWidgets"]
    ),
    LibraryMapping("websockets", "Qt6", "Qt::WebSockets", extra=["COMPONENTS", "WebSockets"]),
    LibraryMapping("webview", "Qt6", "Qt::WebView", extra=["COMPONENTS", "WebView"]),
    LibraryMapping("widgets", "Qt6", "Qt::Widgets", extra=["COMPONENTS", "Widgets"]),
    LibraryMapping("window-lib", "Qt6", "Qt::AppManWindow", extra=["COMPONENTS", "AppManWindow"]),
    LibraryMapping("winextras", "Qt6", "Qt::WinExtras", extra=["COMPONENTS", "WinExtras"]),
    LibraryMapping("x11extras", "Qt6", "Qt::X11Extras", extra=["COMPONENTS", "X11Extras"]),
    LibraryMapping(
        "xcb_qpa_lib", "Qt6", "Qt::XcbQpaPrivate", extra=["COMPONENTS", "XcbQpaPrivate"]
    ),
    LibraryMapping(
        "xkbcommon_support", "Qt6", "Qt::XkbCommonSupport", extra=["COMPONENTS", "XkbCommonSupport"]
    ),
    LibraryMapping("xmlpatterns", "Qt6", "Qt::XmlPatterns", extra=["COMPONENTS", "XmlPatterns"]),
    LibraryMapping("xml", "Qt6", "Qt::Xml", extra=["COMPONENTS", "Xml"]),
    LibraryMapping(
        "qmlworkerscript", "Qt6", "Qt::QmlWorkerScript", extra=["COMPONENTS", "QmlWorkerScript"]
    ),
    LibraryMapping(
        "quickparticles",
        "Qt6",
        "Qt::QuickParticlesPrivate",
        extra=["COMPONENTS", "QuickParticlesPrivate"],
    ),
    LibraryMapping(
        "linuxofono_support",
        "Qt6",
        "Qt::LinuxOfonoSupport",
        extra=["COMPONENTS", "LinuxOfonoSupport"],
    ),
    LibraryMapping(
        "linuxofono_support_private",
        "Qt6",
        "Qt::LinuxOfonoSupportPrivate",
        extra=["COMPONENTS", "LinuxOfonoSupportPrivate"],
    ),
    LibraryMapping("tools", "Qt6", "Qt::Tools", extra=["COMPONENTS", "Tools"]),
    LibraryMapping("axcontainer", "Qt6", "Qt::AxContainer", extra=["COMPONENTS", "AxContainer"]),
    LibraryMapping(
        "webkitwidgets", "Qt6", "Qt::WebKitWidgets", extra=["COMPONENTS", "WebKitWidgets"]
    ),
    LibraryMapping("zlib", "Qt6", "Qt::Zlib", extra=["COMPONENTS", "Zlib"]),
    LibraryMapping("httpserver", "Qt6", "Qt::HttpServer", extra=["COMPONENTS", "HttpServer"]),
    LibraryMapping("sslserver", "Qt6", "Qt::SslServer", extra=["COMPONENTS", "HttpServer"]),
]

# Note that the library map is adjusted dynamically further down.
_library_map = [
    # 3rd party:
    LibraryMapping("atspi", "ATSPI2", "PkgConfig::ATSPI2"),
    LibraryMapping(
        "backtrace", "WrapBacktrace", "WrapBacktrace::WrapBacktrace", emit_if="config.unix"
    ),
    LibraryMapping("bluez", "BlueZ", "PkgConfig::BlueZ"),
    LibraryMapping("brotli", "WrapBrotli", "WrapBrotli::WrapBrotliDec"),
    LibraryMapping("corewlan", None, None),
    LibraryMapping("cups", "Cups", "Cups::Cups"),
    LibraryMapping("directfb", "DirectFB", "PkgConfig::DirectFB"),
    LibraryMapping("db2", "DB2", "DB2::DB2"),
    LibraryMapping("dbus", "WrapDBus1", "dbus-1", resultVariable="DBus1", extra=["1.2"]),
    LibraryMapping(
        "doubleconversion", "WrapSystemDoubleConversion",
        "WrapSystemDoubleConversion::WrapSystemDoubleConversion"
    ),
    LibraryMapping("dlt", "DLT", "DLT::DLT"),
    LibraryMapping("drm", "Libdrm", "Libdrm::Libdrm"),
    LibraryMapping("egl", "EGL", "EGL::EGL"),
    LibraryMapping("flite", "Flite", "Flite::Flite"),
    LibraryMapping("flite_alsa", "ALSA", "ALSA::ALSA"),
    LibraryMapping(
        "fontconfig", "Fontconfig", "Fontconfig::Fontconfig", resultVariable="FONTCONFIG"
    ),
    LibraryMapping(
        "freetype",
        "WrapFreetype",
        "WrapFreetype::WrapFreetype",
        extra=["2.2.0", "REQUIRED"],
        is_bundled_with_qt=True,
    ),
    LibraryMapping("gbm", "gbm", "gbm::gbm"),
    LibraryMapping("glib", "GLIB2", "GLIB2::GLIB2"),
    LibraryMapping("iconv", "WrapIconv", "WrapIconv::WrapIconv"),
    LibraryMapping("gtk3", "GTK3", "PkgConfig::GTK3", extra=["3.6"]),
    LibraryMapping("gssapi", "GSSAPI", "GSSAPI::GSSAPI"),
    LibraryMapping(
        "harfbuzz",
        "WrapHarfbuzz",
        "WrapHarfbuzz::WrapHarfbuzz",
        is_bundled_with_qt=True,
        extra=["2.6.0"],
    ),
    LibraryMapping("host_dbus", None, None),
    LibraryMapping(
        "icu", "ICU", "ICU::i18n ICU::uc ICU::data", extra=["COMPONENTS", "i18n", "uc", "data"]
    ),
    LibraryMapping("journald", "Libsystemd", "PkgConfig::Libsystemd"),
    LibraryMapping("jpeg", "JPEG", "JPEG::JPEG"),  # see also libjpeg
    LibraryMapping("libatomic", "WrapAtomic", "WrapAtomic::WrapAtomic"),
    LibraryMapping("libb2", "Libb2", "Libb2::Libb2"),
    LibraryMapping("libclang", "WrapLibClang", "WrapLibClang::WrapLibClang"),
    LibraryMapping("libdl", None, "${CMAKE_DL_LIBS}"),
    LibraryMapping("libinput", "Libinput", "Libinput::Libinput"),
    LibraryMapping("libjpeg", "JPEG", "JPEG::JPEG"),  # see also jpeg
    LibraryMapping("libpng", "WrapPNG", "WrapPNG::WrapPNG", is_bundled_with_qt=True),
    LibraryMapping("libproxy", "Libproxy", "PkgConfig::Libproxy"),
    LibraryMapping("librt", "WrapRt", "WrapRt::WrapRt"),
    LibraryMapping("libudev", "Libudev", "PkgConfig::Libudev"),
    LibraryMapping("lttng-ust", "LTTngUST", "LTTng::UST", resultVariable="LTTNGUST"),
    LibraryMapping("libmd4c", "WrapMd4c", "WrapMd4c::WrapMd4c", is_bundled_with_qt=True),
    LibraryMapping("mtdev", "Mtdev", "PkgConfig::Mtdev"),
    LibraryMapping("mysql", "MySQL", "MySQL::MySQL"),
    LibraryMapping("odbc", "ODBC", "ODBC::ODBC"),
    LibraryMapping("opengl_es2", "GLESv2", "GLESv2::GLESv2"),
    LibraryMapping("opengl", "WrapOpenGL", "WrapOpenGL::WrapOpenGL", resultVariable="WrapOpenGL"),
    LibraryMapping(
        "openssl_headers",
        "WrapOpenSSLHeaders",
        "WrapOpenSSLHeaders::WrapOpenSSLHeaders",
        resultVariable="TEST_openssl_headers",
        appendFoundSuffix=False,
        test_library_overwrite="WrapOpenSSLHeaders::WrapOpenSSLHeaders",
        run_library_test=True,
    ),
    LibraryMapping(
        "openssl",
        "WrapOpenSSL",
        "WrapOpenSSL::WrapOpenSSL",
        resultVariable="TEST_openssl",
        appendFoundSuffix=False,
        run_library_test=True,
        no_link_so_name="openssl_headers",
    ),
    LibraryMapping("oci", "Oracle", "Oracle::OCI"),
    LibraryMapping(
        "pcre2",
        "WrapPCRE2",
        "WrapPCRE2::WrapPCRE2",
        extra=["10.20", "REQUIRED"],
        is_bundled_with_qt=True,
    ),
    LibraryMapping("pps", "PPS", "PPS::PPS"),
    LibraryMapping("psql", "PostgreSQL", "PostgreSQL::PostgreSQL"),
    LibraryMapping("slog2", "Slog2", "Slog2::Slog2"),
    LibraryMapping("speechd", "SpeechDispatcher", "SpeechDispatcher::SpeechDispatcher"),
    LibraryMapping("sqlite2", None, None),  # No more sqlite2 support in Qt6!
    LibraryMapping("sqlite3", "SQLite3", "SQLite::SQLite3"),
    LibraryMapping("sqlite", "SQLite3", "SQLite::SQLite3"),
    LibraryMapping(
        "taglib", "WrapTagLib", "WrapTagLib::WrapTagLib", is_bundled_with_qt=True
    ),  # used in qtivi
    LibraryMapping("tslib", "Tslib", "PkgConfig::Tslib"),
    LibraryMapping("udev", "Libudev", "PkgConfig::Libudev"),
    LibraryMapping("udev", "Libudev", "PkgConfig::Libudev"),  # see also libudev!
    LibraryMapping("vulkan", "WrapVulkanHeaders", "WrapVulkanHeaders::WrapVulkanHeaders"),
    LibraryMapping("wayland_server", "Wayland", "Wayland::Server"),  # used in qtbase/src/gui
    LibraryMapping("wayland-server", "Wayland", "Wayland::Server"),  # used in qtwayland
    LibraryMapping("wayland-client", "Wayland", "Wayland::Client"),
    LibraryMapping("wayland-cursor", "Wayland", "Wayland::Cursor"),
    LibraryMapping("wayland-egl", "Wayland", "Wayland::Egl"),
    LibraryMapping(
        "wayland-kms", "Waylandkms", "PkgConfig::Waylandkms"
    ),  # TODO: check if this actually works
    LibraryMapping("x11", "X11", "X11::X11"),
    LibraryMapping("x11sm", "X11", "${X11_SM_LIB} ${X11_ICE_LIB}", resultVariable="X11_SM"),
    LibraryMapping(
        "xcb",
        "XCB",
        "XCB::XCB",
        extra=["1.11"],
        resultVariable="TARGET XCB::XCB",
        appendFoundSuffix=False,
    ),
    LibraryMapping(
        "xcb_glx", "XCB", "XCB::GLX", extra=["COMPONENTS", "GLX"], resultVariable="XCB_GLX"
    ),
    LibraryMapping(
        "xcb_icccm",
        "XCB",
        "XCB::ICCCM",
        extra=["0.3.9", "COMPONENTS", "ICCCM"],
        resultVariable="XCB_ICCCM",
    ),
    LibraryMapping(
        "xcb_image",
        "XCB",
        "XCB::IMAGE",
        extra=["0.3.9", "COMPONENTS", "IMAGE"],
        resultVariable="XCB_IMAGE",
    ),
    LibraryMapping(
        "xcb_keysyms",
        "XCB",
        "XCB::KEYSYMS",
        extra=["0.3.9", "COMPONENTS", "KEYSYMS"],
        resultVariable="XCB_KEYSYMS",
    ),
    LibraryMapping(
        "xcb_randr", "XCB", "XCB::RANDR", extra=["COMPONENTS", "RANDR"], resultVariable="XCB_RANDR"
    ),
    LibraryMapping(
        "xcb_render",
        "XCB",
        "XCB::RENDER",
        extra=["COMPONENTS", "RENDER"],
        resultVariable="XCB_RENDER",
    ),
    LibraryMapping(
        "xcb_renderutil",
        "XCB",
        "XCB::RENDERUTIL",
        extra=["0.3.9", "COMPONENTS", "RENDERUTIL"],
        resultVariable="XCB_RENDERUTIL",
    ),
    LibraryMapping(
        "xcb_shape", "XCB", "XCB::SHAPE", extra=["COMPONENTS", "SHAPE"], resultVariable="XCB_SHAPE"
    ),
    LibraryMapping(
        "xcb_shm", "XCB", "XCB::SHM", extra=["COMPONENTS", "SHM"], resultVariable="XCB_SHM"
    ),
    LibraryMapping(
        "xcb_sync", "XCB", "XCB::SYNC", extra=["COMPONENTS", "SYNC"], resultVariable="XCB_SYNC"
    ),
    LibraryMapping(
        "xcb_xfixes",
        "XCB",
        "XCB::XFIXES",
        extra=["COMPONENTS", "XFIXES"],
        resultVariable="TARGET XCB::XFIXES",
        appendFoundSuffix=False,
    ),
    LibraryMapping(
        "xcb-xfixes",
        "XCB",
        "XCB::XFIXES",
        extra=["COMPONENTS", "XFIXES"],
        resultVariable="TARGET XCB::XFIXES",
        appendFoundSuffix=False,
    ),
    LibraryMapping(
        "xcb_xinput",
        "XCB",
        "XCB::XINPUT",
        extra=["1.12", "COMPONENTS", "XINPUT"],
        resultVariable="XCB_XINPUT",
    ),
    LibraryMapping(
        "xcb_xkb", "XCB", "XCB::XKB", extra=["COMPONENTS", "XKB"], resultVariable="XCB_XKB"
    ),
    LibraryMapping("xcb_xlib", "X11_XCB", "X11::XCB"),
    LibraryMapping("xcomposite", "XComposite", "PkgConfig::XComposite"),
    LibraryMapping("xkbcommon_evdev", "XKB", "XKB::XKB", extra=["0.5.0"]),  # see also xkbcommon
    LibraryMapping("xkbcommon_x11", "XKB_COMMON_X11", "PkgConfig::XKB_COMMON_X11", extra=["0.5.0"]),
    LibraryMapping("xkbcommon", "XKB", "XKB::XKB", extra=["0.5.0"]),
    LibraryMapping("xlib", "X11", "X11::X11"),
    LibraryMapping("xrender", "XRender", "PkgConfig::XRender", extra=["0.6"]),
    LibraryMapping("zlib", "WrapZLIB", "WrapZLIB::WrapZLIB", extra=["1.0.8"]),
    LibraryMapping("zstd", "WrapZSTD", "WrapZSTD::WrapZSTD", extra=["1.3"]),
    LibraryMapping("tiff", "TIFF", "TIFF::TIFF"),
    LibraryMapping("webp", "WrapWebP", "WrapWebP::WrapWebP"),
    LibraryMapping("jasper", "WrapJasper", "WrapJasper::WrapJasper"),
    LibraryMapping("mng", "Libmng", "Libmng::Libmng"),
    LibraryMapping("sdl2", "WrapSDL2", "WrapSDL2::WrapSDL2"),
    LibraryMapping("hunspell", "Hunspell", "Hunspell::Hunspell"),
    LibraryMapping(
        "qt3d-assimp",
        "WrapQt3DAssimp",
        "WrapQt3DAssimp::WrapQt3DAssimp",
        extra=["5"],
        run_library_test=True,
        resultVariable="TEST_assimp",
        appendFoundSuffix=False,
    ),
    LibraryMapping(
        "quick3d_assimp",
        "WrapQuick3DAssimp",
        "WrapQuick3DAssimp::WrapQuick3DAssimp",
        extra=["5"],
        run_library_test=True,
        resultVariable="TEST_quick3d_assimp",
        appendFoundSuffix=False,
    ),
]


def _adjust_library_map():
    # Assign a Linux condition on all wayland related packages.
    # Assign platforms that have X11 condition on all X11 related packages.
    # We don't want to get pages of package not found messages on
    # Windows and macOS, and this also improves configure time on
    # those platforms.
    linux_package_prefixes = ["wayland"]
    x11_package_prefixes = ["xcb", "x11", "xkb", "xrender", "xlib"]
    for i, _ in enumerate(_library_map):
        if any([_library_map[i].soName.startswith(p) for p in linux_package_prefixes]):
            _library_map[i].emit_if = "config.linux"
        if any([_library_map[i].soName.startswith(p) for p in x11_package_prefixes]):
            _library_map[i].emit_if = "X11_SUPPORTED"


_adjust_library_map()


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


def find_library_info_for_target(targetName: str) -> typing.Optional[LibraryMapping]:
    qt_target = targetName
    if targetName.endswith("Private"):
        qt_target = qt_target[:-7]

    for i in _qt_library_map:
        if i.targetName == qt_target:
            return i

    for i in _library_map:
        if i.targetName == targetName:
            return i

    return None


# For a given qmake library (e.g. 'openssl_headers'), check whether this is a fake library used
# for the /nolink annotation, and return the actual annotated qmake library ('openssl/nolink').
def find_annotated_qmake_lib_name(lib: str) -> str:
    for entry in _library_map:
        if entry.no_link_so_name == lib:
            return entry.soName + "/nolink"
    return lib


def featureName(name: str) -> str:
    replacement_char = "_"
    if name.startswith("c++"):
        replacement_char = "x"
    return re.sub(r"[^a-zA-Z0-9_]", replacement_char, name)


def map_qt_library(lib: str) -> str:
    private = False
    if lib.endswith("-private"):
        private = True
        lib = lib[:-8]
    mapped = find_qt_library_mapping(lib)
    qt_name = lib
    if mapped:
        assert mapped.targetName  # Qt libs must have a target name set
        qt_name = mapped.targetName
    if private:
        qt_name += "Private"
    return qt_name


platform_mapping = {
    "win32": "WIN32",
    "win": "WIN32",
    "unix": "UNIX",
    "darwin": "APPLE",
    "linux": "LINUX",
    "integrity": "INTEGRITY",
    "qnx": "QNX",
    "vxworks": "VXWORKS",
    "hpux": "HPUX",
    "nacl": "NACL",
    "android": "ANDROID",
    "uikit": "UIKIT",
    "tvos": "TVOS",
    "watchos": "WATCHOS",
    "winrt": "WINRT",
    "wasm": "WASM",
    "emscripten": "EMSCRIPTEN",
    "msvc": "MSVC",
    "clang": "CLANG",
    "gcc": "GCC",
    "icc": "ICC",
    "intel_icc": "ICC",
    "osx": "MACOS",
    "ios": "IOS",
    "freebsd": "FREEBSD",
    "openbsd": "OPENBSD",
    "mingw": "MINGW",
    "netbsd": "NETBSD",
    "haiku": "HAIKU",
    "mac": "APPLE",
    "macx": "MACOS",
    "macos": "MACOS",
    "macx-icc": "(MACOS AND ICC)",
}


def map_platform(platform: str) -> str:
    """ Return the qmake platform as cmake platform or the unchanged string. """
    return platform_mapping.get(platform, platform)


def is_known_3rd_party_library(lib: str) -> bool:
    handling_no_link = False
    if lib.endswith("/nolink") or lib.endswith("_nolink"):
        lib = lib[:-7]
        handling_no_link = True
    mapping = find_3rd_party_library_mapping(lib)
    if handling_no_link and mapping and mapping.no_link_so_name:
        no_link_mapping = find_3rd_party_library_mapping(mapping.no_link_so_name)
        if no_link_mapping:
            mapping = no_link_mapping

    return mapping is not None


def map_3rd_party_library(lib: str) -> str:
    handling_no_link = False
    libpostfix = ""
    if lib.endswith("/nolink"):
        lib = lib[:-7]
        libpostfix = "_nolink"
        handling_no_link = True

    mapping = find_3rd_party_library_mapping(lib)

    if handling_no_link and mapping and mapping.no_link_so_name:
        no_link_mapping = find_3rd_party_library_mapping(mapping.no_link_so_name)
        if no_link_mapping:
            mapping = no_link_mapping
            libpostfix = ""

    if not mapping or not mapping.targetName:
        return lib

    return mapping.targetName + libpostfix


compile_test_dependent_library_mapping = {
    "dtls": {"openssl": "openssl_headers"},
    "ocsp": {"openssl": "openssl_headers"},
}


def get_compile_test_dependent_library_mapping(compile_test_name: str, dependency_name: str):
    if compile_test_name in compile_test_dependent_library_mapping:
        mapping = compile_test_dependent_library_mapping[compile_test_name]
        if dependency_name in mapping:
            return mapping[dependency_name]

    return dependency_name


def generate_find_package_info(
    lib: LibraryMapping,
    use_qt_find_package: bool = True,
    *,
    indent: int = 0,
    emit_if: str = "",
    use_system_package_name: bool = False,
    module: str = "",
) -> str:
    isRequired = False

    extra = lib.extra.copy()

    if "REQUIRED" in extra and use_qt_find_package:
        isRequired = True
        extra.remove("REQUIRED")

    cmake_target_name = lib.targetName
    assert cmake_target_name

    # _nolink or not does not matter at this point:
    if cmake_target_name.endswith("_nolink") or cmake_target_name.endswith("/nolink"):
        cmake_target_name = cmake_target_name[:-7]

    initial_package_name: str = lib.packageName if lib.packageName else ""
    package_name: str = initial_package_name
    if use_system_package_name:
        replace_args = ["Wrap", "WrapSystem"]
        package_name = package_name.replace(*replace_args)  # type: ignore
        cmake_target_name = cmake_target_name.replace(*replace_args)  # type: ignore

    if use_qt_find_package:
        if cmake_target_name:
            extra += ["PROVIDED_TARGETS", cmake_target_name]
        if module:
            extra += ["MODULE_NAME", module]
            extra += ["QMAKE_LIB", find_annotated_qmake_lib_name(lib.soName)]

    result = ""
    one_ind = "    "
    ind = one_ind * indent

    if use_qt_find_package:
        if extra:
            result = f"{ind}qt_find_package({package_name} {' '.join(extra)})\n"
        else:
            result = f"{ind}qt_find_package({package_name})\n"

        if isRequired:
            result += (
                f"{ind}set_package_properties({initial_package_name} PROPERTIES TYPE REQUIRED)\n"
            )
    else:
        if extra:
            result = f"{ind}find_package({package_name} {' '.join(extra)})\n"
        else:
            result = f"{ind}find_package({package_name})\n"

    # If a package should be found only in certain conditions, wrap
    # the find_package call within that condition.
    if emit_if:
        result = f"if(({emit_if}) OR QT_FIND_ALL_PACKAGES_ALWAYS)\n{one_ind}{result}endif()\n"

    return result


def _set_up_py_parsing_nicer_debug_output(pp):
    indent = -1

    def increase_indent(fn):
        def wrapper_function(*args):
            nonlocal indent
            indent += 1
            print("> " * indent, end="")
            return fn(*args)

        return wrapper_function

    def decrease_indent(fn):
        def wrapper_function(*args):
            nonlocal indent
            print("> " * indent, end="")
            indent -= 1
            return fn(*args)

        return wrapper_function

    pp._defaultStartDebugAction = increase_indent(pp._defaultStartDebugAction)
    pp._defaultSuccessDebugAction = decrease_indent(pp._defaultSuccessDebugAction)
    pp._defaultExceptionDebugAction = decrease_indent(pp._defaultExceptionDebugAction)
