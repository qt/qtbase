# Generated from auto.pro.

add_subdirectory(corelib)
if (QT_FEATURE_dbus)
    set(run_dbus_tests ON)
    if(NOT CMAKE_CROSSCOMPILING AND TARGET Qt::DBus)
        execute_process(COMMAND dbus-send --session --type=signal / local.AutotestCheck.Hello
            RESULT_VARIABLE dbus_session_test
            OUTPUT_QUIET ERROR_QUIET)
        if(NOT "${dbus_session_test}" STREQUAL "0")
            set(run_dbus_tests OFF)
            if(QT_FEATURE_dbus_linked)
                message(WARNING
                    " QtDBus is enabled but session bus is not available for testing.\n"
                    " Please check the installation. Skipping QtDBus tests.")
            else()
                message(WARNING
                    " QtDBus is enabled with runtime support, but session bus is not available.\n"
                    " Skipping QtDBus tests.")
            endif()
        endif()
    endif()
    if(run_dbus_tests)
        add_subdirectory(dbus)
    endif()
endif()
if (TARGET Qt::Gui)
    add_subdirectory(gui)
endif()

# special case begin
# Build only corelib and gui tests when targeting uikit (iOS),
# because the script can't handle the SUBDIRS assignment well.
if (APPLE_UIKIT)
    return()
endif()
# special case end

if (TARGET Qt::Network AND NOT WINRT)
     add_subdirectory(network)
endif()
if (TARGET Qt::OpenGL AND NOT WINRT)
     add_subdirectory(opengl)
endif()
if (TARGET Qt::Sql)
     add_subdirectory(sql)
endif()
add_subdirectory(testlib)
if (NOT CMAKE_CROSSCOMPILE AND QT_FEATURE_process)
     add_subdirectory(tools)
endif()
if (TARGET Qt::Xml)
     add_subdirectory(xml)
endif()
if (TARGET Qt::Concurrent)
     add_subdirectory(concurrent)
endif()
add_subdirectory(other)
if (TARGET Qt::Widgets)
     add_subdirectory(widgets)
endif()
if (TARGET Qt::PrintSupport)
     add_subdirectory(printsupport)
endif()
# add_subdirectory(cmake) ## FIXME: Does this still make sense in this form?
# add_subdirectory(installed_cmake) ## FIXME: Does this still make sense in this form?
