# special case begin
cmake_minimum_required(VERSION 3.14.0)

project(QtBaseExamples LANGUAGES CXX C ASM)

# Check whether this project is built as part of a Qt build
if (CMAKE_PROJECT_NAME STREQUAL "QtBaseExamples")
    set(QT_STANDALONE_EXAMPLES_BUILD TRUE)
endif()

if (NOT QT_STANDALONE_EXAMPLES_BUILD)
    # It is part of a Qt build => Use the CMake config files from the binary dir
    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}")
    # Also make sure the CMake config files do not recreate the already-existing targets
    set(QT_NO_CREATE_TARGETS TRUE)
endif()

find_package(Qt5 COMPONENTS DBus Network Test Concurrent Sql Widgets Xml Gui)
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
#         add_subdirectory(opengl) # special case: removed
    endif()

    if(QT_FEATURE_vulkan)
#         add_subdirectory(vulkan) # special case: removed
    endif()
endif()

# special case begin
if (NOT QT_STANDALONE_EXAMPLES_BUILD)
    # We use AUTOMOC/UIC/RCC in the examples. Make sure to not fail on a fresh Qt build, that e.g. the moc binary does not exist yet.

    # This function gets all targets below this directory
    function(get_all_targets _result _dir)
        get_property(_subdirs DIRECTORY "${_dir}" PROPERTY SUBDIRECTORIES)
        foreach(_subdir IN LISTS _subdirs)
            get_all_targets(${_result} "${_subdir}")
        endforeach()
        get_property(_sub_targets DIRECTORY "${_dir}" PROPERTY BUILDSYSTEM_TARGETS)
        set(${_result} ${${_result}} ${_sub_targets} PARENT_SCOPE)
    endfunction()

    get_all_targets(targets "${CMAKE_CURRENT_SOURCE_DIR}")

    foreach(target ${targets})
        qt_autogen_tools(${target} ENABLE_AUTOGEN_TOOLS "moc" "uic" "rcc")
    endforeach()

endif()
# special case end
