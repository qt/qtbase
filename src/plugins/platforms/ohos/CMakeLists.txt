# Copyright (C) 2023 The Qt Company Ltd.
# SPDXicense-Identifier: BSD-3-Clause

#####################################################################
## QOhosPlatformIntegrationPlugin Plugin:
#####################################################################
qt_find_package(EGL MODULE)

qt_internal_add_plugin(QOhosPlatformIntegrationPlugin
    OUTPUT_NAME qohos
    PLUGIN_TYPE platforms
    DEFAULT_IF "ohos" IN_LIST QT_QPA_PLATFORMS
    EXCEPTIONS
    SOURCES
        qarkui/qarkuiutils.h
        qarkui/displaymanager.cpp qarkui/displaymanager.h
        qarkui/input.cpp qarkui/input.h
        qarkui/qembeddedwindownode.cpp qarkui/qembeddedwindownode.h
        qarkui/qnativenodeapi.cpp qarkui/qnativenodeapi.h
        qarkui/qohosdragaction.cpp qarkui/qohosdragaction.h
        qarkui/qqtembeddedwindownode.cpp qarkui/qqtembeddedwindownode.h
        qarkui/qxcomponentregistry.cpp qarkui/qxcomponentregistry.h
        qarkui/vsync.cpp qarkui/vsync.h
        qarkui/window.cpp qarkui/window.h
        qarkui/window_manager.cpp qarkui/window_manager.h
        qohosplatformwindowidstruct_hack_p.h
        qohosapplicationstatetracker.cpp qohosapplicationstatetracker.h
        qohosapppermissions.cpp qohosapppermissions_p.h
        qohosbigdataeventlogging.cpp qohosbigdataeventlogging.h
        qohosclipboardobject.cpp qohosclipboardobject.h
        qohoscloseeventcontext.cpp qohoscloseeventcontext_p.h
        qohosdeadlockprotector.cpp qohosdeadlockprotector.h
        qohosdeviceinfo.cpp qohosdeviceinfo_p.h
        qohosdisplayinfo.cpp qohosdisplayinfo.h
        qohoseglplatformcontext.cpp qohoseglplatformcontext.h
        qohosenums.h
        qohoseventdispatcher.cpp qohoseventdispatcher.h
        qohosfloatingwindow.cpp qohosfloatingwindow.h
        qohosforeignwindow.cpp qohosforeignwindow.h
        qohospermissionshelperimpl.cpp qohospermissionshelperimpl.h
        qohosimageformat.cpp qohosimageformat.h
        qohosinputcontext.cpp qohosinputcontext.h
        qohosinputmethodproxy.cpp qohosinputmethodproxy.h
        qohosinputmethodeventhandler.cpp qohosinputmethodeventhandler.h
        qohosinternalwindowid.cpp qohosinternalwindowid_p.h
        qohosjsenv_p.h
        qohosjsmain.cpp qohosjsmain.h
        qohosjsutils.cpp qohosjsutils.h
        qohoskeyevent.cpp qohoskeyevent.h
        qohoskeyeventconverthelpers.cpp qohoskeyeventconverthelpers.h
        qohoskeymodifiers.cpp qohoskeymodifiers.h
        qohosmimedata.cpp qohosmimedata.h
        qohosmtblockingcallsgateway_p.h
        qohosnativenodekeyevent.cpp qohosnativenodekeyevent.h
        qohosnouichildprocess.cpp qohosnouichildprocess.h
        qohospixelmapconversions.cpp qohospixelmapconversions.h
        qohosplatformbackingstore.cpp qohosplatformbackingstore.h
        qohosplatformbackingstoregl.cpp qohosplatformbackingstoregl.h
        qohosplatformclipboard.cpp qohosplatformclipboard.h
        qohosplatformcursor.cpp qohosplatformcursor.h
        qohosplatformdialoghelper.cpp qohosplatformdialoghelper.h
        qohosplatformdrag.cpp qohosplatformdrag.h
        qohosplatformfiledialoghelper.cpp
        qohosplatformfontdatabase.cpp qohosplatformfontdatabase_p.h
        qohosplatformintegration.cpp qohosplatformintegration.h
        qohosplatformnativeinterface.cpp qohosplatformnativeinterface.h
        qohosplatformplugin.cpp
        qohosplatformscreen.cpp qohosplatformscreen.h
        qohosplatformservices.cpp qohosplatformservices.h
        qohosplatformtheme.cpp qohosplatformtheme.h
        qohosplatformwindow.cpp qohosplatformwindow.h
        qohosplugincore.cpp qohosplugincore.h
        qohosqabilityinstancesmanager.cpp qohosqabilityinstancesmanager.h
        qohosqpafunctionsimpl.cpp qohosqpafunctions_p.h
        qohossharekit.cpp qohossharekit.h
        qohosruntimedevicetypeandmode.cpp qohosruntimedevicetypeandmode.h
        qohosscreenmanager.cpp qohosscreenmanager.h
        qohossettings.cpp qohossettings.h
        qohossinglethreadexecutor.cpp qohossinglethreadexecutor.h
        qohosstatusbarmenu.cpp qohosstatusbarmenu.h
        qohossystemlocale.cpp qohossystemlocale.h
        qohossystemtrayicon.cpp qohossystemtrayicon.h
        qohosudmf.cpp qohosudmf.h
        qohosudmfconversions.cpp qohosudmfconversions.h
        qohosudsobject.cpp qohosudsobject.h
        qohosutils.cpp qohosutils.h
        qohoswatchdog.cpp qohoswatchdog.h
        qohoswindowmanager.cpp qohoswindowmanager.h
        qohosxcomponentkeyevent.cpp qohosxcomponentkeyevent.h
        render/qnativenode.cpp render/qnativenode.h
        render/qohosarkuinativegestureshandler.cpp render/qohosarkuinativegestureshandler.h
        render/qohosbatchingrequestshandler.h
        render/qohosegl.cpp render/qohosegl.h
        render/qohosdrageventutils.cpp render/qohosdrageventutils.h
        render/qohoshovereventsgenerator.cpp render/qohoshovereventsgenerator.h
        render/qohosjswindowregistry.cpp render/qohosjswindowregistry.h
        render/qohosnativeaxiseventhandler.cpp render/qohosnativeaxiseventhandler.h
        render/qohosnativedrageventshandler.cpp render/qohosnativedrageventshandler.h
        render/qohosnativegestureshandler.cpp render/qohosnativegestureshandler.h
        render/qohosnativekeyeventshandler.cpp render/qohosnativekeyeventshandler.h
        render/qohosnativemouseeventshandler.cpp render/qohosnativemouseeventshandler.h
        render/qohosnativexcomponentinputhandler.cpp render/qohosnativexcomponentinputhandler.h
        render/qohossurface.cpp render/qohossurface.h
        render/qohosview.cpp render/qohosview.h
        render/qohoswindowproxy.cpp render/qohoswindowproxy.h
        render/qohoswindowproxydatafactory.cpp render/qohoswindowproxydatafactory.h
        render/qwindowproxyregistry.cpp render/qwindowproxyregistry.h
        render/qxcomponent.cpp render/qxcomponent.h
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
    DEFINES
        NODE_ADDON_API_ENABLE_TYPE_CHECK_ON_AS
        NAPI_CPP_EXCEPTIONS
        QT_NO_CAST_FROM_ASCII
        QT_NO_DYNAMIC_CAST
    LIBRARIES
        NodeAddonApi::NodeAddonApi
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::OpenGL
        Qt::OpenGLPrivate
        Fontconfig::Fontconfig
        EGL::EGL
        ace_napi.z
        ace_ndk.z
        hiappevent_ndk.z
        hilog_ndk.z
        native_display_manager
        native_vsync
        native_window
        native_window_manager
        ohfileshare
        ohfileuri
        ohhicollie
        ohinput
        pasteboard
        pixelmap
        qos
        udmf
        ohinputmethod
)

## Vulkan support
#####################################################################

qt_internal_extend_target(QOhosPlatformIntegrationPlugin
    CONDITION QT_FEATURE_vulkan
    SOURCES
        qohosplatformvulkaninstance.cpp qohosplatformvulkaninstance.h
        render/qohosvulkansurface.cpp render/qohosvulkansurface.h
    DEFINES
        VK_USE_PLATFORM_OHOS=1
)

