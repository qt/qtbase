# Please note the target_dep_list accepts not the actual list values but the list names that
# contain preformed dependencies. See foreach block for reference.
# The same applies for find_dependency_path_list.
macro(_qt_internal_find_dependencies target_dep_list find_dependency_path_list)
    foreach(__qt_target_dep IN LISTS ${target_dep_list})
        list(GET __qt_target_dep 0 __qt_pkg)
        list(GET __qt_target_dep 1 __qt_version)

        if (NOT ${__qt_pkg}_FOUND)
            set(__qt_pkg_names ${__qt_pkg})
            if(__qt_pkg MATCHES "(.*)Private$")
                set(__qt_pkg_names "${CMAKE_MATCH_1};${__qt_pkg}")
            endif()
            find_dependency(${__qt_pkg} ${__qt_version}
                NAMES
                    ${__qt_pkg_names}
                PATHS
                    ${${find_dependency_path_list}}
                    ${_qt_additional_packages_prefix_paths}
                    ${QT_EXAMPLES_CMAKE_PREFIX_PATH}
                ${__qt_use_no_default_path_for_qt_packages}
            )
        endif()
    endforeach()
endmacro()
