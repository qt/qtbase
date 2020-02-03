# Generated from windowsuiautomation.pro.

#####################################################################
## WindowsUIAutomationSupport Module:
#####################################################################

qt_add_module(WindowsUIAutomationSupport
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
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
)

#### Keys ignored in scope 1:.:.:windowsuiautomation.pro:<TRUE>:
# MODULE = "windowsuiautomation_support"
