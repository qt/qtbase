# Generated from qtexttableborders.pro.

#####################################################################
## qtexttableborders Binary:
#####################################################################

qt_internal_add_manual_test(qtexttableborders
    GUI
    SOURCES
        main.cpp
        widget.cpp widget.h widget.ui
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::PrintSupport
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)

# Resources:
set(resources_resource_files
    "table-border-test-header.html"
    "table-border-test.html"
)

qt_internal_add_resource(qtexttableborders "resources"
    PREFIX
        "/"
    FILES
        ${resources_resource_files}
)


#### Keys ignored in scope 1:.:.:qtexttableborders.pro:<TRUE>:
# TEMPLATE = "app"
