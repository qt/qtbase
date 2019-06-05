# Generated from windowsuiautomation.pro.

#####################################################################
## WindowsUIAutomationSupport Module:
#####################################################################

add_qt_module(WindowsUIAutomationSupport
    STATIC
    INTERNAL_MODULE
    SOURCES
        qwindowsuiawrapper.cpp qwindowsuiawrapper_p.h
        uiaattributeids_p.h
        uiaclientinterfaces_p.h
        uiacontroltypeids_p.h
        uiaerrorids_p.h
        uiaeventids_p.h
        uiageneralids_p.h
        uiapatternids_p.h
        uiapropertyids_p.h
        uiaserverinterfaces_p.h
        uiatypes_p.h
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
)

#### Keys ignored in scope 1:.:.:windowsuiautomation.pro:<TRUE>:
# CONFIG = "static" "internal_module"
# MODULE = "windowsuiautomation_support"
# _LOADED = "qt_module"
