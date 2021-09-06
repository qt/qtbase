# Generated from qaccessibility.pro.

if(NOT QT_FEATURE_accessibility)
    return()
endif()

# special case begin
if (WIN32 AND NOT TARGET Qt::WindowsUIAutomationSupport)
    return()
endif()
# special case end

#####################################################################
## tst_qaccessibility Test:
#####################################################################

qt_internal_add_test(tst_qaccessibility
    SOURCES
        accessiblewidgets.h
        tst_qaccessibility.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::WidgetsPrivate
)

#### Keys ignored in scope 1:.:.:qaccessibility.pro:<TRUE>:
# _REQUIREMENTS = "qtConfig(accessibility)"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qaccessibility CONDITION UNIX AND NOT APPLE AND NOT HAIKU AND NOT INTEGRITY
    PUBLIC_LIBRARIES
        m
)

qt_internal_extend_target(tst_qaccessibility CONDITION WIN32
    PUBLIC_LIBRARIES
        ole32
        oleacc
        oleaut32
        uuid
)
