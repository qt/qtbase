# special case begin
qt_examples_build_begin()

# special case end

# Generated from examples.pro.

add_subdirectory(corelib)
add_subdirectory(embedded)
add_subdirectory(qpa)

if(TARGET Qt::DBus)
    add_subdirectory(dbus)
endif()

if(TARGET Qt::Network)
    add_subdirectory(network)
endif()

if(TARGET Qt::Test)
    add_subdirectory(qtestlib)
endif()

if(TARGET Qt::Concurrent)
    add_subdirectory(qtconcurrent)
endif()

if(TARGET Qt::Sql)
    add_subdirectory(sql)
endif()

if(TARGET Qt::Widgets)
    add_subdirectory(widgets)
endif()

if(TARGET Qt::Xml)
    add_subdirectory(xml)
endif()

if(TARGET Qt::Gui)
    add_subdirectory(gui)

    if(QT_FEATURE_opengl)
        add_subdirectory(opengl)
    endif()

    if(QT_FEATURE_vulkan)
        add_subdirectory(vulkan)
    endif()
endif()

# special case begin
qt_examples_build_end()
# special case end
