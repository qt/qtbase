# Generated from ios.pro.

#####################################################################
## qios Plugin:
#####################################################################

add_qt_plugin(qios
    TYPE platforms
    CLASS_NAME QIOSIntegrationPlugin
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
    LIBRARIES
        Qt::ClipboardSupportPrivate
        Qt::CorePrivate
        Qt::FontDatabaseSupportPrivate
        Qt::GraphicsSupportPrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        ${FWAudioToolbox}
        ${FWFoundation}
        ${FWQuartzCore}
        ${FWUIKit}
        Qt::ClipboardSupport
        Qt::Core
        Qt::FontDatabaseSupport
        Qt::GraphicsSupport
        Qt::Gui
)

#### Keys ignored in scope 2:.:.:kernel.pro:<TRUE>:
# OTHER_FILES = "quiview_textinput.mm" "quiview_accessibility.mm"
# PLUGIN_CLASS_NAME = "QIOSIntegrationPlugin"
# PLUGIN_TYPE = "platforms"
# _LOADED = "qt_plugin"

## Scopes:
#####################################################################

#### Keys ignored in scope 3:.:.:kernel.pro:QT_FEATURE_shared:
# CONFIG = "static"

extend_target(qios CONDITION NOT APPLE_TVOS
    SOURCES
        qiosclipboard.h qiosclipboard.mm
        qiosfiledialog.h qiosfiledialog.mm
        qiosmenu.h qiosmenu.mm
        qiosmessagedialog.h qiosmessagedialog.mm
        qiostextinputoverlay.h qiostextinputoverlay.mm
    PUBLIC_LIBRARIES
        ${FWAssetsLibrary}
)

#### Keys ignored in scope 5:.:.:kernel.pro:NOT TARGET___equals____ss_QT_DEFAULT_QPA_PLUGIN:
# PLUGIN_EXTENDS = "-"
add_subdirectory(optional)
