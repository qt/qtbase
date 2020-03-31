# Generated from kernel.pro.

#####################################################################
## QIOSIntegrationPlugin Plugin:
#####################################################################

add_qt_plugin(QIOSIntegrationPlugin
    OUTPUT_NAME qios
    TYPE platforms
    SOURCES
        plugin.mm
        qiosapplicationdelegate.h qiosapplicationdelegate.mm
        qiosapplicationstate.h qiosapplicationstate.mm
        qiosbackingstore.h qiosbackingstore.mm
        qioscontext.h qioscontext.mm
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
    PUBLIC_LIBRARIES
        ${FWAudioToolbox}
        ${FWFoundation}
        ${FWQuartzCore}
        ${FWUIKit}
        Qt::Core
        Qt::CorePrivate
        Qt::FontDatabaseSupportPrivate
        Qt::GraphicsSupportPrivate
        Qt::Gui
        Qt::GuiPrivate
)

#### Keys ignored in scope 1:.:.:kernel.pro:<TRUE>:
# OTHER_FILES = "quiview_textinput.mm" "quiview_accessibility.mm"

## Scopes:
#####################################################################

extend_target(QIOSIntegrationPlugin CONDITION TARGET Qt::PlatformCompositorSupportPrivate
    PUBLIC_LIBRARIES
        Qt::PlatformCompositorSupportPrivate
)

extend_target(QIOSIntegrationPlugin CONDITION NOT TVOS
    SOURCES
        qiosclipboard.h qiosclipboard.mm
        qiosdocumentpickercontroller.h qiosdocumentpickercontroller.mm
        qiosfiledialog.h qiosfiledialog.mm
        qiosmenu.h qiosmenu.mm
        qiosmessagedialog.h qiosmessagedialog.mm
        qiostextinputoverlay.h qiostextinputoverlay.mm
    PUBLIC_LIBRARIES
        ${FWAssetsLibrary}
)

#### Keys ignored in scope 5:.:.:kernel.pro:NOT TARGET___equals____ss_QT_DEFAULT_QPA_PLUGIN:
# PLUGIN_EXTENDS = "-"
