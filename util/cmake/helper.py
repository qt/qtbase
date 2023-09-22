# Copyright (C) 2021 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
        components: typing.Optional[typing.List[str]] = None,
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
        self.components = components
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
    LibraryMapping("androidextras", "Qt6", "Qt::AndroidExtras", components=["AndroidExtras"]),
    LibraryMapping("3danimation", "Qt6", "Qt::3DAnimation", components=["3DAnimation"]),
    LibraryMapping("3dcore", "Qt6", "Qt::3DCore", components=["3DCore"]),
    LibraryMapping("3dcoretest", "Qt6", "Qt::3DCoreTest", components=["3DCoreTest"]),
    LibraryMapping("3dextras", "Qt6", "Qt::3DExtras", components=["3DExtras"]),
    LibraryMapping("3dinput", "Qt6", "Qt::3DInput", components=["3DInput"]),
    LibraryMapping("3dlogic", "Qt6", "Qt::3DLogic", components=["3DLogic"]),
    LibraryMapping("3dquick", "Qt6", "Qt::3DQuick", components=["3DQuick"]),
    LibraryMapping("3dquickextras", "Qt6", "Qt::3DQuickExtras", components=["3DQuickExtras"]),
    LibraryMapping("3dquickinput", "Qt6", "Qt::3DQuickInput", components=["3DQuickInput"]),
    LibraryMapping("3dquickrender", "Qt6", "Qt::3DQuickRender", components=["3DQuickRender"]),
    LibraryMapping("3drender", "Qt6", "Qt::3DRender", components=["3DRender"]),
    LibraryMapping(
        "application-lib", "Qt6", "Qt::AppManApplication", components=["AppManApplication"]
    ),
    LibraryMapping("axbase", "Qt6", "Qt::AxBasePrivate", components=["AxBasePrivate"]),
    LibraryMapping("axcontainer", "Qt6", "Qt::AxContainer", components=["AxContainer"]),
    LibraryMapping("axserver", "Qt6", "Qt::AxServer", components=["AxServer"]),
    LibraryMapping("bluetooth", "Qt6", "Qt::Bluetooth", components=["Bluetooth"]),
    LibraryMapping("bootstrap", "Qt6", "Qt::Bootstrap", components=["Bootstrap"]),
    # bootstrap-dbus: Not needed in Qt6!
    LibraryMapping("client", "Qt6", "Qt::WaylandClient", components=["WaylandClient"]),
    LibraryMapping("coap", "Qt6", "Qt::Coap", components=["Coap"]),
    LibraryMapping("common-lib", "Qt6", "Qt::AppManCommon", components=["AppManCommon"]),
    LibraryMapping("compositor", "Qt6", "Qt::WaylandCompositor", components=["WaylandCompositor"]),
    LibraryMapping("concurrent", "Qt6", "Qt::Concurrent", components=["Concurrent"]),
    LibraryMapping("container", "Qt6", "Qt::AxContainer", components=["AxContainer"]),
    LibraryMapping("control", "Qt6", "Qt::AxServer", components=["AxServer"]),
    LibraryMapping("core_headers", "Qt6", "Qt::WebEngineCore", components=["WebEngineCore"]),
    LibraryMapping("core", "Qt6", "Qt::Core", components=["Core"]),
    LibraryMapping("crypto-lib", "Qt6", "Qt::AppManCrypto", components=["AppManCrypto"]),
    LibraryMapping("dbus", "Qt6", "Qt::DBus", components=["DBus"]),
    LibraryMapping("designer", "Qt6", "Qt::Designer", components=["Designer"]),
    LibraryMapping(
        "designercomponents",
        "Qt6",
        "Qt::DesignerComponentsPrivate",
        components=["DesignerComponentsPrivate"],
    ),
    LibraryMapping(
        "devicediscovery",
        "Qt6",
        "Qt::DeviceDiscoverySupportPrivate",
        components=["DeviceDiscoverySupportPrivate"],
    ),
    LibraryMapping(
        "devicediscovery_support",
        "Qt6",
        "Qt::DeviceDiscoverySupportPrivate",
        components=["DeviceDiscoverySupportPrivate"],
    ),
    LibraryMapping("edid", "Qt6", "Qt::EdidSupport", components=["EdidSupport"]),
    LibraryMapping("edid_support", "Qt6", "Qt::EdidSupport", components=["EdidSupport"]),
    LibraryMapping("eglconvenience", "Qt6", "Qt::EglSupport", components=["EglSupport"]),
    LibraryMapping(
        "eglfsdeviceintegration",
        "Qt6",
        "Qt::EglFSDeviceIntegrationPrivate",
        components=["EglFSDeviceIntegrationPrivate"],
    ),
    LibraryMapping(
        "eglfs_kms_support",
        "Qt6",
        "Qt::EglFsKmsSupportPrivate",
        components=["EglFsKmsSupportPrivate"],
    ),
    LibraryMapping(
        "eglfs_kms_gbm_support",
        "Qt6",
        "Qt::EglFsKmsGbmSupportPrivate",
        components=["EglFsKmsGbmSupportPrivate"],
    ),
    LibraryMapping("egl_support", "Qt6", "Qt::EglSupport", components=["EglSupport"]),
    # enginio: Not needed in Qt6!
    LibraryMapping(
        "eventdispatchers",
        "Qt6",
        "Qt::EventDispatcherSupport",
        components=["EventDispatcherSupport"],
    ),
    LibraryMapping(
        "eventdispatcher_support",
        "Qt6",
        "Qt::EventDispatcherSupport",
        components=["EventDispatcherSupport"],
    ),
    LibraryMapping("fbconvenience", "Qt6", "Qt::FbSupportPrivate", components=["FbSupportPrivate"]),
    LibraryMapping("fb_support", "Qt6", "Qt::FbSupportPrivate", components=["FbSupportPrivate"]),
    LibraryMapping(
        "fontdatabase_support",
        "Qt6",
        "Qt::FontDatabaseSupport",
        components=["FontDatabaseSupport"],
    ),
    LibraryMapping("gamepad", "Qt6", "Qt::Gamepad", components=["Gamepad"]),
    LibraryMapping("geniviextras", "Qt6", "Qt::GeniviExtras", components=["GeniviExtras"]),
    LibraryMapping("global", "Qt6", "Qt::Core", components=["Core"]),  # manually added special case
    LibraryMapping("glx_support", "Qt6", "Qt::GlxSupport", components=["GlxSupport"]),
    LibraryMapping("gsttools", "Qt6", "Qt::MultimediaGstTools", components=["MultimediaGstTools"]),
    LibraryMapping("gui", "Qt6", "Qt::Gui", components=["Gui"]),
    LibraryMapping("help", "Qt6", "Qt::Help", components=["Help"]),
    LibraryMapping(
        "hunspellinputmethod",
        "Qt6",
        "Qt::HunspellInputMethodPrivate",
        components=["HunspellInputMethodPrivate"],
    ),
    LibraryMapping("input", "Qt6", "Qt::InputSupportPrivate", components=["InputSupportPrivate"]),
    LibraryMapping(
        "input_support",
        "Qt6",
        "Qt::InputSupportPrivate",
        components=["InputSupportPrivate"],
    ),
    LibraryMapping("installer-lib", "Qt6", "Qt::AppManInstaller", components=["AppManInstaller"]),
    LibraryMapping("ivi", "Qt6", "Qt::Ivi", components=["Ivi"]),
    LibraryMapping("ivicore", "Qt6", "Qt::IviCore", components=["IviCore"]),
    LibraryMapping("ivimedia", "Qt6", "Qt::IviMedia", components=["IviMedia"]),
    LibraryMapping("knx", "Qt6", "Qt::Knx", components=["Knx"]),
    LibraryMapping(
        "kmsconvenience", "Qt6", "Qt::KmsSupportPrivate", components=["KmsSupportPrivate"]
    ),
    LibraryMapping("kms_support", "Qt6", "Qt::KmsSupportPrivate", components=["KmsSupportPrivate"]),
    LibraryMapping("launcher-lib", "Qt6", "Qt::AppManLauncher", components=["AppManLauncher"]),
    LibraryMapping("lib", "Qt6", "Qt::Designer", components=["Designer"]),
    LibraryMapping(
        "linuxaccessibility_support",
        "Qt6",
        "Qt::LinuxAccessibilitySupport",
        components=["LinuxAccessibilitySupport"],
    ),
    LibraryMapping("location", "Qt6", "Qt::Location", components=["Location"]),
    LibraryMapping("macextras", "Qt6", "Qt::MacExtras", components=["MacExtras"]),
    LibraryMapping("main-lib", "Qt6", "Qt::AppManMain", components=["AppManMain"]),
    LibraryMapping("manager-lib", "Qt6", "Qt::AppManManager", components=["AppManManager"]),
    LibraryMapping("monitor-lib", "Qt6", "Qt::AppManMonitor", components=["AppManMonitor"]),
    LibraryMapping("mqtt", "Qt6", "Qt::Mqtt", components=["Mqtt"]),
    LibraryMapping("multimedia", "Qt6", "Qt::Multimedia", components=["Multimedia"]),
    LibraryMapping(
        "multimediawidgets",
        "Qt6",
        "Qt::MultimediaWidgets",
        components=["MultimediaWidgets"],
    ),
    LibraryMapping("network", "Qt6", "Qt::Network", components=["Network"]),
    LibraryMapping("networkauth", "Qt6", "Qt::NetworkAuth", components=["NetworkAuth"]),
    LibraryMapping("nfc", "Qt6", "Qt::Nfc", components=["Nfc"]),
    LibraryMapping("oauth", "Qt6", "Qt::NetworkAuth", components=["NetworkAuth"]),
    LibraryMapping("opcua", "Qt6", "Qt::OpcUa", components=["OpcUa"]),
    LibraryMapping("opcua_private", "Qt6", "Qt::OpcUaPrivate", components=["OpcUaPrivate"]),
    LibraryMapping("opengl", "Qt6", "Qt::OpenGL", components=["OpenGL"]),
    LibraryMapping("openglwidgets", "Qt6", "Qt::OpenGLWidgets", components=["OpenGLWidgets"]),
    LibraryMapping("package-lib", "Qt6", "Qt::AppManPackage", components=["AppManPackage"]),
    LibraryMapping(
        "packetprotocol",
        "Qt6",
        "Qt::PacketProtocolPrivate",
        components=["PacketProtocolPrivate"],
    ),
    LibraryMapping(
        "particles",
        "Qt6",
        "Qt::QuickParticlesPrivate",
        components=["QuickParticlesPrivate"],
    ),
    LibraryMapping(
        "plugin-interfaces",
        "Qt6",
        "Qt::AppManPluginInterfaces",
        components=["AppManPluginInterfaces"],
    ),
    LibraryMapping("positioning", "Qt6", "Qt::Positioning", components=["Positioning"]),
    LibraryMapping(
        "positioningquick", "Qt6", "Qt::PositioningQuick", components=["PositioningQuick"]
    ),
    LibraryMapping("printsupport", "Qt6", "Qt::PrintSupport", components=["PrintSupport"]),
    LibraryMapping("purchasing", "Qt6", "Qt::Purchasing", components=["Purchasing"]),
    LibraryMapping("qmldebug", "Qt6", "Qt::QmlDebugPrivate", components=["QmlDebugPrivate"]),
    LibraryMapping(
        "qmldevtools", "Qt6", "Qt::QmlDevToolsPrivate", components=["QmlDevToolsPrivate"]
    ),
    LibraryMapping(
        "qmlcompiler", "Qt6", "Qt::QmlCompilerPrivate", components=["QmlCompilerPrivate"]
    ),
    LibraryMapping("qml", "Qt6", "Qt::Qml", components=["Qml"]),
    LibraryMapping("qmldom", "Qt6", "Qt::QmlDomPrivate", components=["QmlDomPrivate"]),
    LibraryMapping("qmlmodels", "Qt6", "Qt::QmlModels", components=["QmlModels"]),
    LibraryMapping("qmltest", "Qt6", "Qt::QuickTest", components=["QuickTest"]),
    LibraryMapping(
        "qtmultimediaquicktools",
        "Qt6",
        "Qt::MultimediaQuickPrivate",
        components=["MultimediaQuickPrivate"],
    ),
    LibraryMapping(
        "quick3dassetimport",
        "Qt6",
        "Qt::Quick3DAssetImport",
        components=["Quick3DAssetImport"],
    ),
    LibraryMapping("core5compat", "Qt6", "Qt::Core5Compat", components=["Core5Compat"]),
    LibraryMapping("quick3d", "Qt6", "Qt::Quick3D", components=["Quick3D"]),
    LibraryMapping("quick3drender", "Qt6", "Qt::Quick3DRender", components=["Quick3DRender"]),
    LibraryMapping(
        "quick3druntimerender",
        "Qt6",
        "Qt::Quick3DRuntimeRender",
        components=["Quick3DRuntimeRender"],
    ),
    LibraryMapping("quick3dutils", "Qt6", "Qt::Quick3DUtils", components=["Quick3DUtils"]),
    LibraryMapping("quickcontrols2", "Qt6", "Qt::QuickControls2", components=["QuickControls2"]),
    LibraryMapping(
        "quickcontrols2impl",
        "Qt6",
        "Qt::QuickControls2Impl",
        components=["QuickControls2Impl"],
    ),
    LibraryMapping("quick", "Qt6", "Qt::Quick", components=["Quick"]),
    LibraryMapping(
        "quickshapes", "Qt6", "Qt::QuickShapesPrivate", components=["QuickShapesPrivate"]
    ),
    LibraryMapping("quicktemplates2", "Qt6", "Qt::QuickTemplates2", components=["QuickTemplates2"]),
    LibraryMapping("quickwidgets", "Qt6", "Qt::QuickWidgets", components=["QuickWidgets"]),
    LibraryMapping("remoteobjects", "Qt6", "Qt::RemoteObjects", components=["RemoteObjects"]),
    LibraryMapping("script", "Qt6", "Qt::Script", components=["Script"]),
    LibraryMapping("scripttools", "Qt6", "Qt::ScriptTools", components=["ScriptTools"]),
    LibraryMapping("scxml", "Qt6", "Qt::Scxml", components=["Scxml"]),
    LibraryMapping("sensors", "Qt6", "Qt::Sensors", components=["Sensors"]),
    LibraryMapping("serialport", "Qt6", "Qt::SerialPort", components=["SerialPort"]),
    LibraryMapping("serialbus", "Qt6", "Qt::SerialBus", components=["SerialBus"]),
    LibraryMapping("services", "Qt6", "Qt::ServiceSupport", components=["ServiceSupport"]),
    LibraryMapping("service_support", "Qt6", "Qt::ServiceSupport", components=["ServiceSupport"]),
    LibraryMapping("shadertools", "Qt6", "Qt::ShaderTools", components=["ShaderTools"]),
    LibraryMapping("statemachine", "Qt6", "Qt::StateMachine", components=["StateMachine"]),
    LibraryMapping("sql", "Qt6", "Qt::Sql", components=["Sql"]),
    LibraryMapping("svg", "Qt6", "Qt::Svg", components=["Svg"]),
    LibraryMapping("svgwidgets", "Qt6", "Qt::SvgWidgets", components=["SvgWidgets"]),
    LibraryMapping("charts", "Qt6", "Qt::Charts", components=["Charts"]),
    LibraryMapping("testlib", "Qt6", "Qt::Test", components=["Test"]),
    LibraryMapping("texttospeech", "Qt6", "Qt::TextToSpeech", components=["TextToSpeech"]),
    LibraryMapping("theme_support", "Qt6", "Qt::ThemeSupport", components=["ThemeSupport"]),
    LibraryMapping("tts", "Qt6", "Qt::TextToSpeech", components=["TextToSpeech"]),
    LibraryMapping("uiplugin", "Qt6", "Qt::UiPlugin", components=["UiPlugin"]),
    LibraryMapping("uitools", "Qt6", "Qt::UiTools", components=["UiTools"]),
    LibraryMapping("virtualkeyboard", "Qt6", "Qt::VirtualKeyboard", components=["VirtualKeyboard"]),
    LibraryMapping("waylandclient", "Qt6", "Qt::WaylandClient", components=["WaylandClient"]),
    LibraryMapping(
        "waylandcompositor",
        "Qt6",
        "Qt::WaylandCompositor",
        components=["WaylandCompositor"],
    ),
    LibraryMapping("webchannel", "Qt6", "Qt::WebChannel", components=["WebChannel"]),
    LibraryMapping("webengine", "Qt6", "Qt::WebEngine", components=["WebEngine"]),
    LibraryMapping(
        "webenginewidgets", "Qt6", "Qt::WebEngineWidgets", components=["WebEngineWidgets"]
    ),
    LibraryMapping("websockets", "Qt6", "Qt::WebSockets", components=["WebSockets"]),
    LibraryMapping("webview", "Qt6", "Qt::WebView", components=["WebView"]),
    LibraryMapping("widgets", "Qt6", "Qt::Widgets", components=["Widgets"]),
    LibraryMapping("window-lib", "Qt6", "Qt::AppManWindow", components=["AppManWindow"]),
    LibraryMapping("winextras", "Qt6", "Qt::WinExtras", components=["WinExtras"]),
    LibraryMapping("x11extras", "Qt6", "Qt::X11Extras", components=["X11Extras"]),
    LibraryMapping("xcb_qpa_lib", "Qt6", "Qt::XcbQpaPrivate", components=["XcbQpaPrivate"]),
    LibraryMapping(
        "xkbcommon_support", "Qt6", "Qt::XkbCommonSupport", components=["XkbCommonSupport"]
    ),
    LibraryMapping("xmlpatterns", "Qt6", "Qt::XmlPatterns", components=["XmlPatterns"]),
    LibraryMapping("xml", "Qt6", "Qt::Xml", components=["Xml"]),
    LibraryMapping("qmlworkerscript", "Qt6", "Qt::QmlWorkerScript", components=["QmlWorkerScript"]),
    LibraryMapping(
        "quickparticles",
        "Qt6",
        "Qt::QuickParticlesPrivate",
        components=["QuickParticlesPrivate"],
    ),
    LibraryMapping(
        "linuxofono_support",
        "Qt6",
        "Qt::LinuxOfonoSupport",
        components=["LinuxOfonoSupport"],
    ),
    LibraryMapping(
        "linuxofono_support_private",
        "Qt6",
        "Qt::LinuxOfonoSupportPrivate",
        components=["LinuxOfonoSupportPrivate"],
    ),
    LibraryMapping("tools", "Qt6", "Qt::Tools", components=["Tools"]),
    LibraryMapping("axcontainer", "Qt6", "Qt::AxContainer", components=["AxContainer"]),
    LibraryMapping("webkitwidgets", "Qt6", "Qt::WebKitWidgets", components=["WebKitWidgets"]),
    LibraryMapping("zlib", "Qt6", "Qt::Zlib", components=["Zlib"]),
    LibraryMapping("httpserver", "Qt6", "Qt::HttpServer", components=["HttpServer"]),
    LibraryMapping("sslserver", "Qt6", "Qt::SslServer", components=["HttpServer"]),
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
    LibraryMapping("icu", "ICU", "ICU::i18n ICU::uc ICU::data", components=["i18n", "uc", "data"]),
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
    LibraryMapping("xcb_glx", "XCB", "XCB::GLX", components=["GLX"], resultVariable="XCB_GLX"),
    LibraryMapping(
        "xcb_cursor",
        "XCB",
        "XCB::CURSOR",
        extra=["0.1.1", "COMPONENTS", "CURSOR"],
        resultVariable="XCB_CURSOR",
    ),
    LibraryMapping(
        "xcb_icccm",
        "XCB",
        "XCB::ICCCM",
        extra=["0.3.9"],
        components=["ICCCM"],
        resultVariable="XCB_ICCCM",
    ),
    LibraryMapping(
        "xcb_image",
        "XCB",
        "XCB::IMAGE",
        extra=["0.3.9"],
        components=["IMAGE"],
        resultVariable="XCB_IMAGE",
    ),
    LibraryMapping(
        "xcb_keysyms",
        "XCB",
        "XCB::KEYSYMS",
        extra=["0.3.9"],
        components=["KEYSYMS"],
        resultVariable="XCB_KEYSYMS",
    ),
    LibraryMapping(
        "xcb_randr", "XCB", "XCB::RANDR", components=["RANDR"], resultVariable="XCB_RANDR"
    ),
    LibraryMapping(
        "xcb_render",
        "XCB",
        "XCB::RENDER",
        components=["RENDER"],
        resultVariable="XCB_RENDER",
    ),
    LibraryMapping(
        "xcb_renderutil",
        "XCB",
        "XCB::RENDERUTIL",
        extra=["0.3.9"],
        components=["RENDERUTIL"],
        resultVariable="XCB_RENDERUTIL",
    ),
    LibraryMapping(
        "xcb_shape", "XCB", "XCB::SHAPE", components=["SHAPE"], resultVariable="XCB_SHAPE"
    ),
    LibraryMapping("xcb_shm", "XCB", "XCB::SHM", components=["SHM"], resultVariable="XCB_SHM"),
    LibraryMapping("xcb_sync", "XCB", "XCB::SYNC", components=["SYNC"], resultVariable="XCB_SYNC"),
    LibraryMapping(
        "xcb_xfixes",
        "XCB",
        "XCB::XFIXES",
        components=["XFIXES"],
        resultVariable="TARGET XCB::XFIXES",
        appendFoundSuffix=False,
    ),
    LibraryMapping(
        "xcb-xfixes",
        "XCB",
        "XCB::XFIXES",
        components=["XFIXES"],
        resultVariable="TARGET XCB::XFIXES",
        appendFoundSuffix=False,
    ),
    LibraryMapping(
        "xcb_xinput",
        "XCB",
        "XCB::XINPUT",
        extra=["1.12"],
        components=["XINPUT"],
        resultVariable="XCB_XINPUT",
    ),
    LibraryMapping("xcb_xkb", "XCB", "XCB::XKB", components=["XKB"], resultVariable="XCB_XKB"),
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
    """Return the qmake platform as cmake platform or the unchanged string."""
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
    remove_REQUIRED_from_extra: bool = True,
    components_required: bool = True,
    module: str = "",
) -> str:
    isRequired = False

    extra = lib.extra.copy()
    if lib.components:
        extra.append("COMPONENTS" if components_required else "OPTIONAL_COMPONENTS")
        extra += lib.components

    if "REQUIRED" in extra and use_qt_find_package:
        isRequired = True
        if remove_REQUIRED_from_extra:
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

    if hasattr(pp, "_defaultStartDebugAction"):
        pp._defaultStartDebugAction = increase_indent(pp._defaultStartDebugAction)
        pp._defaultSuccessDebugAction = decrease_indent(pp._defaultSuccessDebugAction)
        pp._defaultExceptionDebugAction = decrease_indent(pp._defaultExceptionDebugAction)
    elif hasattr(pp.core, "_default_start_debug_action"):
        pp.core._default_start_debug_action = increase_indent(pp.core._default_start_debug_action)
        pp.core._default_success_debug_action = decrease_indent(
            pp.core._default_success_debug_action
        )
        pp.core._default_exception_debug_action = decrease_indent(
            pp.core._default_exception_debug_action
        )
