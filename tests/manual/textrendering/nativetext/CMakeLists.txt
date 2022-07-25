# Generated from nativetext.pro.

#####################################################################
## nativetext Binary:
#####################################################################

qt_internal_add_manual_test(nativetext
    SOURCES
        main.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
)

## Scopes:
#####################################################################

qt_internal_extend_target(nativetext CONDITION APPLE
    LIBRARIES
        ${FWAppKit}
        ${FWCoreGraphics}
        ${FWFoundation}
    COMPILE_OPTIONS
        -x
        objective-c++
)
