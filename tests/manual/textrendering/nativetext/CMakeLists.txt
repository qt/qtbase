# Generated from nativetext.pro.

#####################################################################
## nativetext Binary:
#####################################################################

add_qt_manual_test(nativetext
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

extend_target(nativetext CONDITION APPLE
    PUBLIC_LIBRARIES
        ${FWAppKit}
        ${FWCoreGraphics}
        ${FWFoundation}
    COMPILE_OPTIONS
        -x
        objective-c++
)
