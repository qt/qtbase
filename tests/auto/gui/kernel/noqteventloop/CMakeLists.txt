# Generated from noqteventloop.pro.

#####################################################################
## tst_noqteventloop Test:
#####################################################################

qt_internal_add_test(tst_noqteventloop
    SOURCES
        tst_noqteventloop.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Network
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_noqteventloop CONDITION QT_FEATURE_dynamicgl AND WIN32
    PUBLIC_LIBRARIES
        user32
)
