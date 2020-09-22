# Generated from nativetext.pro.

#####################################################################
## nativetext Binary:
#####################################################################

qt_internal_add_manual_test(nativetext
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
)

## Scopes:
#####################################################################

qt_internal_extend_target(nativetext CONDITION APPLE
    PUBLIC_LIBRARIES
        ${FWAppKit}
        ${FWCoreGraphics}
        ${FWFoundation}
    COMPILE_OPTIONS
        -x
        objective-c++
)
