# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QIOSIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QIOSIntegrationPlugin
    OUTPUT_NAME qios
    STATIC # Force static, even in shared builds
    DEFAULT_IF ${QT_QPA_DEFAULT_PLATFORM} MATCHES ios
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
        uistrings_p.h uistrings.cpp
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

qt_internal_extend_target(QIOSIntegrationPlugin CONDITION NOT TVOS
    SOURCES
        qiosclipboard.h qiosclipboard.mm
        qiosdocumentpickercontroller.h qiosdocumentpickercontroller.mm
        qiosfiledialog.h qiosfiledialog.mm
        qioscolordialog.h qioscolordialog.mm
        qiosfontdialog.h qiosfontdialog.mm
        qiosmenu.h qiosmenu.mm
        qiosmessagedialog.h qiosmessagedialog.mm
        qiostextinputoverlay.h qiostextinputoverlay.mm
    LIBRARIES
        ${FWAssetsLibrary}
        ${FWUniformTypeIdentifiers}
        ${FWPhotos}
)

add_subdirectory(optional)
