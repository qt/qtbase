qt_internal_add_manual_test(cursors
    GUI
    SOURCES
        MainWindow.cpp MainWindow.h MainWindow.ui
        main.cpp
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)
