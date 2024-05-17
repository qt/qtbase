# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QIOSIntegrationPlugin Plugin:
#####################################################################

if(VISIONOS)
    include(SwiftIntegration.cmake)

    qt_install(TARGETS QIOSIntegrationPluginSwift
        EXPORT "${INSTALL_CMAKE_NAMESPACE}QIOSIntegrationPluginTargets"
        DESTINATION "${INSTALL_LIBDIR}"
    )
    qt_internal_add_targets_to_additional_targets_export_file(
        TARGETS QIOSIntegrationPluginSwift
        EXPORT_NAME_PREFIX "${INSTALL_CMAKE_NAMESPACE}QIOSIntegrationPlugin"
    )
endif()

qt_internal_add_plugin(QIOSIntegrationPlugin
    OUTPUT_NAME qios
    STATIC # Force static, even in shared builds
    DEFAULT_IF "ios" IN_LIST QT_QPA_PLATFORMS
    PLUGIN_TYPE platforms
    SOURCES
        plugin.mm
        qiosapplicationdelegate.h qiosapplicationdelegate.mm
        qiosapplicationstate.h qiosapplicationstate.mm
        qioseventdispatcher.h qioseventdispatcher.mm
        qiosglobal.h qiosglobal.mm
        qiosinputcontext.h qiosinputcontext.mm
        qiosintegration.h qiosintegration.mm
        qiosplatformaccessibility.h qiosplatformaccessibility.mm
        qiosscreen.h qiosscreen.mm
        qiosservices.h qiosservices.mm
        qiostextresponder.h qiostextresponder.mm
        qiostheme.h qiostheme.mm
        qiosviewcontroller.h qiosviewcontroller.mm
        qioswindow.h qioswindow.mm
        quiaccessibilityelement.h quiaccessibilityelement.mm
        quiview.h quiview.mm
        quiwindow.mm quiwindow.h
        uistrings_p.h uistrings.cpp
    NO_PCH_SOURCES
        qioscontext.mm # undef QT_NO_FOREACH
        qiosintegration.mm # undef QT_NO_FOREACH
        qiosplatformaccessibility.mm # undef QT_NO_FOREACH
        qiosscreen.mm # undef QT_NO_FOREACH
    LIBRARIES
        ${FWAudioToolbox}
        ${FWFoundation}
        ${FWMetal}
        ${FWQuartzCore}
        ${FWUIKit}
        ${FWCoreGraphics}
        Qt::CorePrivate
        Qt::GuiPrivate
)

qt_disable_apple_app_extension_api_only(QIOSIntegrationPlugin)

## Scopes:
#####################################################################
qt_internal_find_apple_system_framework(FWUniformTypeIdentifiers UniformTypeIdentifiers)

qt_internal_extend_target(QIOSIntegrationPlugin CONDITION QT_FEATURE_opengl
    SOURCES
        qioscontext.h qioscontext.mm
    LIBRARIES
        Qt::OpenGLPrivate
)

qt_internal_extend_target(QIOSIntegrationPlugin CONDITION QT_FEATURE_clipboard
    SOURCES
        qiosclipboard.h qiosclipboard.mm
)

qt_internal_extend_target(QIOSIntegrationPlugin CONDITION NOT TVOS
    SOURCES
        qiosfiledialog.h qiosfiledialog.mm
        qiosdocumentpickercontroller.h qiosdocumentpickercontroller.mm
    LIBRARIES
        ${FWUniformTypeIdentifiers}
        ${FWPhotos}
)

qt_internal_extend_target(QIOSIntegrationPlugin CONDITION NOT TVOS
    SOURCES
        qioscolordialog.h qioscolordialog.mm
        qiosfontdialog.h qiosfontdialog.mm
        qiosmessagedialog.h qiosmessagedialog.mm
)

qt_internal_extend_target(QIOSIntegrationPlugin CONDITION NOT (TVOS OR VISIONOS)
    SOURCES
        qiosmenu.h qiosmenu.mm
        qiostextinputoverlay.h qiostextinputoverlay.mm
)

add_subdirectory(optional)

if(VISIONOS)
    target_link_libraries(QIOSIntegrationPlugin PRIVATE QIOSIntegrationPluginSwift)
endif()
