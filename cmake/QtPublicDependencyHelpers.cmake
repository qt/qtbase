# Note that target_dep_list does not accept a list of values, but a var name that contains the
# list of dependencies. See foreach block for reference.
macro(_qt_internal_find_third_party_dependencies target target_dep_list)
    foreach(__qt_${target}_target_dep IN LISTS ${target_dep_list})
        list(GET __qt_${target}_target_dep 0 __qt_${target}_pkg)
        list(GET __qt_${target}_target_dep 1 __qt_${target}_is_optional)
        list(GET __qt_${target}_target_dep 2 __qt_${target}_version)
        list(GET __qt_${target}_target_dep 3 __qt_${target}_components)
        list(GET __qt_${target}_target_dep 4 __qt_${target}_optional_components)
        set(__qt_${target}_find_package_args "${__qt_${target}_pkg}")
        if(__qt_${target}_version)
            list(APPEND __qt_${target}_find_package_args "${__qt_${target}_version}")
        endif()
        if(__qt_${target}_components)
            string(REPLACE " " ";" __qt_${target}_components "${__qt_${target}_components}")
            list(APPEND __qt_${target}_find_package_args COMPONENTS ${__qt_${target}_components})
        endif()
        if(__qt_${target}_optional_components)
            string(REPLACE " " ";"
                __qt_${target}_optional_components "${__qt_${target}_optional_components}")
            list(APPEND __qt_${target}_find_package_args
                 OPTIONAL_COMPONENTS ${__qt_${target}_optional_components})
        endif()

        if(__qt_${target}_is_optional)
            if(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY)
                list(APPEND __qt_${target}_find_package_args QUIET)
            endif()
            find_package(${__qt_${target}_find_package_args})
        else()
            find_dependency(${__qt_${target}_find_package_args})
        endif()
    endforeach()
endmacro()

# Note that target_dep_list does not accept a list of values, but a var name that contains the
# list of dependencies. See foreach block for reference.
macro(_qt_internal_find_tool_dependencies target target_dep_list)
    if(NOT "${${target_dep_list}}" STREQUAL "" AND NOT "${QT_HOST_PATH}" STREQUAL "")
         # Make sure that the tools find the host tools first
         set(BACKUP_${target}_CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})
         set(BACKUP_${target}_CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH})
         list(PREPEND CMAKE_PREFIX_PATH "${QT_HOST_PATH_CMAKE_DIR}"
             "${_qt_additional_host_packages_prefix_paths}")
         list(PREPEND CMAKE_FIND_ROOT_PATH "${QT_HOST_PATH}"
             "${_qt_additional_host_packages_root_paths}")
    endif()

    foreach(__qt_${target}_target_dep IN LISTS ${target_dep_list})
        list(GET __qt_${target}_target_dep 0 __qt_${target}_pkg)
        list(GET __qt_${target}_target_dep 1 __qt_${target}_version)

        unset(__qt_${target}_find_package_args)
        if(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY)
            list(APPEND __qt_${target}_find_package_args QUIET)
        endif()
        find_package(${__qt_${target}_pkg}
            ${__qt_${target}_version}
            ${__qt_${target}_find_package_args}
            PATHS
                "${CMAKE_CURRENT_LIST_DIR}/.."
                "${_qt_cmake_dir}"
                ${_qt_additional_packages_prefix_paths}
        )
        if (NOT ${__qt_${target}_pkg}_FOUND)
            set(${CMAKE_FIND_PACKAGE_NAME}_FOUND FALSE)
            set(${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_MESSAGE
"${CMAKE_FIND_PACKAGE_NAME} could not be found because dependency \
${__qt_${target}_pkg} could not be found.")
            if(NOT "${QT_HOST_PATH}" STREQUAL "")
                 set(CMAKE_PREFIX_PATH ${BACKUP_${target}_CMAKE_PREFIX_PATH})
                 set(CMAKE_FIND_ROOT_PATH ${BACKUP_${target}_CMAKE_FIND_ROOT_PATH})
            endif()
            return()
        endif()
    endforeach()
    if(NOT "${${target_dep_list}}" STREQUAL "" AND NOT "${QT_HOST_PATH}" STREQUAL "")
         set(CMAKE_PREFIX_PATH ${BACKUP_${target}_CMAKE_PREFIX_PATH})
         set(CMAKE_FIND_ROOT_PATH ${BACKUP_${target}_CMAKE_FIND_ROOT_PATH})
    endif()
endmacro()

# Please note the target_dep_list accepts not the actual list values but the list names that
# contain preformed dependencies. See foreach block for reference.
# The same applies for find_dependency_path_list.
macro(_qt_internal_find_qt_dependencies target target_dep_list find_dependency_path_list)
    foreach(__qt_${target}_target_dep IN LISTS ${target_dep_list})
        list(GET __qt_${target}_target_dep 0 __qt_${target}_pkg)
        list(GET __qt_${target}_target_dep 1 __qt_${target}_version)

        if (NOT ${__qt_${target}_pkg}_FOUND)
            set(__qt_${target}_pkg_names ${__qt_${target}_pkg})
            if(__qt_${target}_pkg MATCHES "(.*)Private$")
                set(__qt_${target}_pkg_names "${CMAKE_MATCH_1};${__qt_${target}_pkg}")
            endif()
            find_dependency(${__qt_${target}_pkg} ${__qt_${target}_version}
                NAMES
                    ${__qt_${target}_pkg_names}
                PATHS
                    ${${find_dependency_path_list}}
                    ${_qt_additional_packages_prefix_paths}
                    ${QT_EXAMPLES_CMAKE_PREFIX_PATH}
                ${__qt_use_no_default_path_for_qt_packages}
            )
        endif()
    endforeach()
endmacro()


# TODO: Remove once a dependency update completes and most developers have the Dependencies.cmake
# files updated in their builds.
# The name is too generic, it doesn't look for any kind of dependencies but only Qt package
# dependencies.
macro(_qt_internal_find_dependencies target_dep_list find_dependency_path_list)
    _qt_internal_find_qt_dependencies("none" "${target_dep_list}" "${find_dependency_path_list}")
endmacro()

# If a dependency package was not found, provide some hints in the error message on how to debug
# the issue.
# pkg_name_var should be the variable name that contains the package that was not found.
# e.g. __qt_Core_pkg
# message_out_var should contain the variable name of the  original "not found" message, and it
# will have the hints appended to it as a string. e.g. ${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_MESSAGE
#
# The function should not be called in Dependencies.cmake files directly, because find_dependency
# returns out of the included file.
macro(_qt_internal_suggest_dependency_debugging pkg_name_var message_out_var)
    if(${pkg_name_var}
        AND NOT ${CMAKE_FIND_PACKAGE_NAME}_FOUND
        AND ${message_out_var})
        if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.23")
            string(APPEND ${message_out_var}
                "\nConfiguring with --debug-find-pkg=${${pkg_name_var}} might reveal \
details why the package was not found.")
        elseif(CMAKE_VERSION VERSION_GREATER_EQUAL "3.17")
            string(APPEND ${message_out_var}
                "\nConfiguring with -DCMAKE_FIND_DEBUG_MODE=TRUE might reveal \
details why the package was not found.")
        endif()
    endif()
endmacro()
