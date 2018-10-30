add_qt_tool(qdbusxml2cpp
    SOURCES qdbusxml2cpp.cpp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_FOREACH
    LIBRARIES
        Qt::CorePrivate
        Qt::DBusPrivate
)
