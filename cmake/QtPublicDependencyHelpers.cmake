# Please note the target_dep_list accepts not the actual list values but the list names that
# contain preformed dependencies. See foreach block for reference.
# The same applies for find_dependency_path_list.
macro(_qt_internal_find_dependencies target_dep_list find_dependency_path_list)
    foreach(target_dep IN LISTS ${target_dep_list})
        list(GET target_dep 0 pkg)
        list(GET target_dep 1 version)

        if (NOT ${pkg}_FOUND)
            set(pkg_names ${pkg})
            if(pkg MATCHES "(.*)Private$")
                set(pkg_names "${CMAKE_MATCH_1};${pkg}")
            endif()
            find_dependency(${pkg} ${version}
                NAMES
                    ${pkg_names}
                PATHS
                    ${${find_dependency_path_list}}
                    ${_qt_additional_packages_prefix_path}
                    ${_qt_additional_packages_prefix_path_env}
                    ${QT_EXAMPLES_CMAKE_PREFIX_PATH}
                ${__qt_use_no_default_path_for_qt_packages}
            )
        endif()
    endforeach()
endmacro()
