#
#  W A R N I N G
#  -------------
#
# This file is not part of the Qt API.  It exists purely as an
# implementation detail.  This file, and its contents may change from version to
# version without notice, or even be removed.
#
# We mean it.


set(BUILD_OPTIONS_LIST)

if (CMAKE_BUILD_TYPE)
  list(APPEND BUILD_OPTIONS_LIST "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
endif()

if (CMAKE_TOOLCHAIN_FILE)
  list(APPEND BUILD_OPTIONS_LIST "-DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
endif()

macro(expect_pass _dir)
  string(REPLACE "(" "_" testname "${_dir}")
  string(REPLACE ")" "_" testname "${testname}")
  add_test(${testname} ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMAKE_CURRENT_SOURCE_DIR}/${_dir}"
    "${CMAKE_CURRENT_BINARY_DIR}/${_dir}"
    --build-config "${CMAKE_BUILD_TYPE}"
    --build-generator ${CMAKE_GENERATOR}
    --build-makeprogram ${CMAKE_MAKE_PROGRAM}
    --build-project ${_dir}
    --build-options "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}" ${BUILD_OPTIONS_LIST}
  )
endmacro()

macro(expect_fail _dir)
  string(REPLACE "(" "_" testname "${_dir}")
  string(REPLACE ")" "_" testname "${testname}")
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/failbuild/${_dir}")
  file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/${_dir}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/failbuild/${_dir}")

  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/failbuild/${_dir}/${_dir}/FindPackageHints.cmake" "set(Qt5Tests_PREFIX_PATH \"${CMAKE_PREFIX_PATH}\")")

  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/failbuild/${_dir}/CMakeLists.txt"
    "
      cmake_minimum_required(VERSION 2.8)
      project(${_dir})

      try_compile(Result \${CMAKE_CURRENT_BINARY_DIR}/${_dir}
          \${CMAKE_CURRENT_SOURCE_DIR}/${_dir}
          ${_dir}
          OUTPUT_VARIABLE Out
      )
      message(\"\${Out}\")
      if (Result)
        message(SEND_ERROR \"Succeeded build which should fail\")
      endif()
      "
  )
  add_test(${testname} ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMAKE_CURRENT_BINARY_DIR}/failbuild/${_dir}"
    "${CMAKE_CURRENT_BINARY_DIR}/failbuild/${_dir}/build"
    --build-config "${CMAKE_BUILD_TYPE}"
    --build-generator ${CMAKE_GENERATOR}
    --build-makeprogram ${CMAKE_MAKE_PROGRAM}
    --build-project ${_dir}
    --build-options "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}" ${BUILD_OPTIONS_LIST}
  )
endmacro()

function(test_module_includes)

  set(all_args ${ARGN})
  set(packages_string "")
  set(libraries_string "")

  foreach(_package ${Qt5_MODULE_TEST_DEPENDS})
    set(packages_string
      "
      ${packages_string}
      find_package(Qt5${_package} REQUIRED)
      "
    )
  endforeach()

  while(all_args)
    list(GET all_args 0 qtmodule)
    list(REMOVE_AT all_args 0 1)
    set(packages_string
      "${packages_string}
      find_package(Qt5${qtmodule} REQUIRED)
      include_directories(\${Qt5${qtmodule}_INCLUDE_DIRS})
      add_definitions(\${Qt5${qtmodule}_DEFINITIONS})\n"
    )
    set(libraries_string "${libraries_string} Qt5::${qtmodule}")
  endwhile()

  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/module_includes/CMakeLists.txt"
    "
      cmake_minimum_required(VERSION 2.8)
      project(module_includes)

      ${packages_string}

      set(CMAKE_CXX_FLAGS \"\${CMAKE_CXX_FLAGS} \${Qt5Core_EXECUTABLE_COMPILE_FLAGS}\")

      add_executable(module_includes_exe \"\${CMAKE_CURRENT_SOURCE_DIR}/main.cpp\")
      target_link_libraries(module_includes_exe ${libraries_string})\n"
  )

  set(all_args ${ARGN})
  set(includes_string "")
  set(instances_string "")
  while(all_args)
    list(GET all_args 0 qtmodule)
    list(GET all_args 1 qtinclude)
    list(REMOVE_AT all_args 0 1)
    set(includes_string
      "${includes_string}
      #include <${qtinclude}>
      #include <Qt${qtmodule}/${qtinclude}>
      #include <Qt${qtmodule}>
      #include <Qt${qtmodule}/Qt${qtmodule}>"
    )
    set(instances_string
    "${instances_string}
    ${qtinclude} local${qtinclude};
    ")
  endwhile()

  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/module_includes/main.cpp"
    "

    ${includes_string}

    int main(int, char **) { ${instances_string} return 0; }\n"
  )

  add_test(module_includes ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMAKE_CURRENT_BINARY_DIR}/module_includes/"
    "${CMAKE_CURRENT_BINARY_DIR}/module_includes/build"
    --build-config "${CMAKE_BUILD_TYPE}"
    --build-generator ${CMAKE_GENERATOR}
    --build-makeprogram ${CMAKE_MAKE_PROGRAM}
    --build-project module_includes
    --build-options "-DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}" ${BUILD_OPTIONS_LIST}
  )
endfunction()
