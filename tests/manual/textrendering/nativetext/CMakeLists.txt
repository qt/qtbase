# Generated from nativetext.pro.

#####################################################################
## nativetext Binary:
#####################################################################

add_qt_manual_test(nativetext
    SOURCES
        main.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
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
