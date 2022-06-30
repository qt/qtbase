#####################################################################
## tst_wasm Test:
#####################################################################

qt_internal_add_test(tst_localfileapi
    SOURCES
        tst_localfileapi.cpp
    DEFINES
        QT_NO_FOREACH
        QT_NO_KEYWORDS
    LIBRARIES
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Widgets
)
