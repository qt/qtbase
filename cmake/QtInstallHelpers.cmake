# Wraps install() command. In a prefix build, simply passes along arguments to install().
# In a non-prefix build, handles association of targets to export names, and also calls export().
function(qt_install)
    set(flags)
    set(options EXPORT DESTINATION NAMESPACE)
    set(multiopts TARGETS)
    cmake_parse_arguments(arg "${flags}" "${options}" "${multiopts}" ${ARGN})

    if(arg_TARGETS)
        set(is_install_targets TRUE)
    endif()

    # In a prefix build, always invoke install() without modification.
    # In a non-prefix build, pass install(TARGETS) commands to allow
    # association of targets to export names, so we can later use the export names
    # in export() commands.
    if(QT_WILL_INSTALL OR is_install_targets)
        install(${ARGV})
    endif()

    # Exit early if this is a prefix build.
    if(QT_WILL_INSTALL)
        return()
    endif()

    # In a non-prefix build, when install(EXPORT) is called,
    # also call export(EXPORT) to generate build tree target files.
    if(NOT is_install_targets AND arg_EXPORT)
        set(namespace_option "")
        if(arg_NAMESPACE)
            set(namespace_option NAMESPACE ${arg_NAMESPACE})
        endif()
        export(EXPORT ${arg_EXPORT}
               ${namespace_option}
               FILE "${arg_DESTINATION}/${arg_EXPORT}.cmake")
    endif()
endfunction()

# Copies files using file(COPY) signature in non-prefix builds.
function(qt_non_prefix_copy)
    if(NOT QT_WILL_INSTALL)
        file(${ARGV})
    endif()
endfunction()

# Retrieve the permissions that are set by install(PROGRAMS).
function(qt_get_install_executable_permissions out_var)
    set(default_permissions ${CMAKE_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS})
    if(NOT default_permissions)
        set(default_permissions OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
    endif()
    set(executable_permissions ${default_permissions} OWNER_EXECUTE)
    if(GROUP_READ IN_LIST default_permissions)
        list(APPEND executable_permissions GROUP_EXECUTE)
    endif()
    if(WORLD_READ IN_LIST default_permissions)
        list(APPEND executable_permissions WORLD_EXECUTE)
    endif()
    set(${out_var} ${executable_permissions} PARENT_SCOPE)
endfunction()

# Use case is installing files in a prefix build, or copying them to the correct build dir
# in a non-prefix build.
# Pass along arguments as you would pass them to install().
# Only supports FILES, PROGRAMS and DIRECTORY signature, and without fancy things
# like OPTIONAL or RENAME or COMPONENT.
function(qt_copy_or_install)
    set(flags FILES PROGRAMS DIRECTORY)
    set(options)
    set(multiopts)
    cmake_parse_arguments(arg "${flags}" "${options}" "${multiopts}" ${ARGN})

    # Remember which option has to be passed to the install command.
    set(copy_arguments "")
    set(argv_copy ${ARGV})
    if(arg_FILES)
        set(install_option "FILES")
    elseif(arg_PROGRAMS)
        set(install_option "PROGRAMS")
        qt_get_install_executable_permissions(executable_permissions)
        list(APPEND copy_arguments FILE_PERMISSIONS ${executable_permissions})
    elseif(arg_DIRECTORY)
        set(install_option "DIRECTORY")
    endif()

    list(REMOVE_AT argv_copy 0)
    qt_install(${install_option} ${argv_copy})
    qt_non_prefix_copy(COPY ${argv_copy} ${copy_arguments})
endfunction()

# Hacky way to remove the install target in non-prefix builds.
# We need to associate targets with export names, and that is only possible to do with the
# install(TARGETS) command. But in a non-prefix build, we don't want to install anything.
# To make sure that developers don't accidentally run make install, replace the generated
# cmake_install.cmake file with an empty file. To do this, always create a new temporary file
# at CMake configuration step, and use it as an input to a custom command that replaces the
# cmake_install.cmake file with an empty one. This means we will always replace the file on
# every reconfiguration, but not when doing null builds.
function(qt_remove_install_target)
    # On superbuilds we only do this for qtbase - it will correctly remove the
    # cmake_install.cmake at the root of the repository.
    if(QT_SUPERBUILD)
      if(NOT (PROJECT_NAME STREQUAL "QtBase"))
        return()
      endif()
    endif()

    set(file_in "${CMAKE_BINARY_DIR}/.remove_cmake_install_in.txt")
    set(file_generated "${CMAKE_BINARY_DIR}/.remove_cmake_install_generated.txt")
    set(cmake_install_file "${CMAKE_BINARY_DIR}/cmake_install.cmake")
    file(WRITE ${file_in} "")

    add_custom_command(OUTPUT ${file_generated}
        COMMAND ${CMAKE_COMMAND} -E copy ${file_in} ${file_generated}
        COMMAND ${CMAKE_COMMAND} -E remove ${cmake_install_file}
        COMMAND ${CMAKE_COMMAND} -E touch ${cmake_install_file}
        COMMENT "Removing cmake_install.cmake"
        MAIN_DEPENDENCY ${file_in})

    add_custom_target(remove_cmake_install ALL DEPENDS ${file_generated})
endfunction()

function(qt_set_up_nonprefix_build)
    if(NOT QT_WILL_INSTALL)
        qt_remove_install_target()
    endif()
endfunction()
