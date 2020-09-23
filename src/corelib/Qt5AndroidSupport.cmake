if (NOT ${PROJECT_NAME}-MultiAbiBuild)

  set(ANDROID_ABIS armeabi-v7a arm64-v8a x86 x86_64)

  # Match Android's sysroots
  set(ANDROID_SYSROOT_armeabi-v7a arm-linux-androideabi)
  set(ANDROID_SYSROOT_arm64-v8a aarch64-linux-android)
  set(ANDROID_SYSROOT_x86 i686-linux-android)
  set(ANDROID_SYSROOT_x86_64 x86_64-linux-android)

  foreach(abi IN LISTS ANDROID_ABIS)
    set(abi_initial_value OFF)
    if (abi STREQUAL ${ANDROID_ABI})
      set(abi_initial_value ON)
    endif()
    find_library(Qt5Core_${abi}_Probe Qt5Core_${abi})
    if (Qt5Core_${abi}_Probe)
      option(ANDROID_BUILD_ABI_${abi} "Enable the build for Android ${abi}" ${abi_initial_value})
    endif()
  endforeach()
  option(ANDROID_MIN_SDK_VERSION "Android minimum SDK version" "21")
  option(ANDROID_TARGET_SDK_VERSION "Android target SDK version" "28")

  # Make sure to delete the "android-build" directory, which contains all the
  # build artefacts, and also the androiddeployqt/gradle artefacts
  set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    APPEND PROPERTY ADDITIONAL_CLEAN_FILES ${CMAKE_BINARY_DIR}/android-build)

  if (CMAKE_VERSION VERSION_LESS 3.15)
    message(STATUS "-----------------------------------------------------------------------------------------------------------")
    message(STATUS "CMake version 3.15 is required to clean the <build_dir>/android-build when issuing the \"clean\" target.\n\n"
      "For this CMake version please use the \"clean-android-build\" support target additionally to the \"clean\" target.")
    message(STATUS "-----------------------------------------------------------------------------------------------------------")

    add_custom_target(clean-android-build
      COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/android-build
      VERBATIM)
  endif()

  # Write the android_<project_name>_deployment_settings.json file
  file(WRITE ${CMAKE_BINARY_DIR}/android_deployment_settings.json.in
[=[{
  "_description": "This file is created by CMake to be read by androiddeployqt and should not be modified by hand.",
  "application-binary": "@QT_ANDROID_APPLICATION_BINARY@",
  "architectures": {
    @QT_ANDROID_ARCHITECTURES@
  },
  @QT_ANDROID_DEPLOYMENT_DEPENDENCIES@
  @QT_ANDROID_EXTRA_PLUGINS@
  @QT_ANDROID_PACKAGE_SOURCE_DIR@
  @QT_ANDROID_VERSION_CODE@
  @QT_ANDROID_VERSION_NAME@
  @QT_ANDROID_EXTRA_LIBS@
  @QT_QML_IMPORT_PATH@
  "ndk": "@ANDROID_NDK@",
  "ndk-host": "@ANDROID_HOST_TAG@",
  "qml-root-path": "@CMAKE_CURRENT_SOURCE_DIR@",
  "qt": "@QT_DIR@",
  "sdk": "@ANDROID_SDK@",
  "stdcpp-path": "@ANDROID_TOOLCHAIN_ROOT@/sysroot/usr/lib/",
  "tool-prefix": "llvm",
  "toolchain-prefix": "llvm",
  "useLLVM": true
}]=])

  if (NOT QT_ANDROID_APPLICATION_BINARY)
    set(QT_ANDROID_APPLICATION_BINARY ${PROJECT_NAME})
  endif()

  if(NOT ANDROID_SDK)
    get_filename_component(ANDROID_SDK ${ANDROID_NDK}/../ ABSOLUTE)
  endif()

  find_program(ANDROID_DEPLOY_QT androiddeployqt)
  get_filename_component(QT_DIR ${ANDROID_DEPLOY_QT}/../../ ABSOLUTE)

  unset(QT_ANDROID_ARCHITECTURES)
  foreach(abi IN LISTS ANDROID_ABIS)
    if (ANDROID_BUILD_ABI_${abi})
      list(APPEND QT_ANDROID_ARCHITECTURES "\"${abi}\" : \"${ANDROID_SYSROOT_${abi}}\"")
    endif()
  endforeach()
  string(REPLACE ";" ",\n" QT_ANDROID_ARCHITECTURES "${QT_ANDROID_ARCHITECTURES}")

  macro(generate_json_variable_list var_list json_key)
    if (${var_list})
      set(QT_${var_list} "\"${json_key}\": \"")
      string(REPLACE ";" "," joined_var_list "${${var_list}}")
      string(APPEND QT_${var_list} "${joined_var_list}\",")
    endif()
  endmacro()

  macro(generate_json_variable var json_key)
    if (${var})
      set(QT_${var} "\"${json_key}\": \"${${var}}\",")
    endif()
  endmacro()

  generate_json_variable_list(ANDROID_DEPLOYMENT_DEPENDENCIES "deployment-dependencies")
  generate_json_variable_list(ANDROID_EXTRA_PLUGINS "android-extra-plugins")
  generate_json_variable(ANDROID_PACKAGE_SOURCE_DIR "android-package-source-directory")
  generate_json_variable(ANDROID_VERSION_CODE "android-version-code")
  generate_json_variable(ANDROID_VERSION_NAME "android-version-name")
  generate_json_variable_list(ANDROID_EXTRA_LIBS "android-extra-libs")
  generate_json_variable_list(QML_IMPORT_PATH "qml-import-paths")
  generate_json_variable_list(ANDROID_MIN_SDK_VERSION "android-min-sdk-version")
  generate_json_variable_list(ANDROID_TARGET_SDK_VERSION "android-target-sdk-version")


  configure_file(
    "${CMAKE_BINARY_DIR}/android_deployment_settings.json.in"
    "${CMAKE_BINARY_DIR}/android_deployment_settings.json" @ONLY)

  # Create "apk" and "aab" targets
  if (DEFINED ENV{JAVA_HOME})
    set(JAVA_HOME $ENV{JAVA_HOME} CACHE INTERNAL "Saved JAVA_HOME variable")
  endif()
  if (JAVA_HOME)
    set(android_deploy_qt_jdk "--jdk ${JAVA_HOME}")
  endif()

  if (ANDROID_SDK_PLATFORM)
    set(android_deploy_qt_platform "--android-platform ${ANDROID_SDK_PLATFORM}")
  endif()

  add_custom_target(apk
    COMMAND ${CMAKE_COMMAND} -E env JAVA_HOME=${JAVA_HOME} ${ANDROID_DEPLOY_QT}
       --input "${CMAKE_BINARY_DIR}/android_deployment_settings.json"
       --output "${CMAKE_BINARY_DIR}/android-build"
       --apk "${CMAKE_BINARY_DIR}/android-build/${PROJECT_NAME}.apk"
       ${android_deploy_qt_platform}
       ${android_deploy_qt_jdk}
    VERBATIM)

  add_custom_target(aab
    COMMAND ${CMAKE_COMMAND} -E env JAVA_HOME=${JAVA_HOME} ${ANDROID_DEPLOY_QT}
      --input "${CMAKE_BINARY_DIR}/android_deployment_settings.json"
      --output "${CMAKE_BINARY_DIR}/android-build"
      --apk "${CMAKE_BINARY_DIR}/android-build/${PROJECT_NAME}.apk"
      --aab
      ${android_deploy_qt_platform}
      ${android_deploy_qt_jdk}
   VERBATIM)

  include(ExternalProject)
  macro (setup_library library_name android_abi)
    # Use Qt Creator's 4.12 settings file if present
    unset(QTC_SETTINGS_PARAMETER)
    if (EXISTS ${CMAKE_BINARY_DIR}/qtcsettings.cmake)
      set(QTC_SETTINGS_PARAMETER -C ${CMAKE_BINARY_DIR}/qtcsettings.cmake)
    endif()

    # Build all the given ABI as an external project
    ExternalProject_Add(${library_name}-builder
      SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
      PREFIX MultiAbi
      BUILD_ALWAYS YES
      DOWNLOAD_COMMAND ""
      INSTALL_COMMAND ""
      UPDATE_COMMAND ""
      PATCH_COMMAND ""
      CMAKE_ARGS
        ${QTC_SETTINGS_PARAMETER}
        -D ANDROID_ABI=${android_abi}
        -D CMAKE_FIND_ROOT_PATH=${CMAKE_FIND_ROOT_PATH}
        -D CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}
        -D CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -D ANDROID_PLATFORM=${ANDROID_PLATFORM}
        -D ANDROID_NATIVE_API_LEVEL=${ANDROID_NATIVE_API_LEVEL}
        -D CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
        -D CMAKE_FIND_ROOT_PATH_MODE_PROGRAM=${CMAKE_FIND_ROOT_PATH_MODE_PROGRAM}
        -D CMAKE_FIND_ROOT_PATH_MODE_LIBRARY=${CMAKE_FIND_ROOT_PATH_MODE_LIBRARY}
        -D CMAKE_FIND_ROOT_PATH_MODE_INCLUDE=${CMAKE_FIND_ROOT_PATH_MODE_INCLUDE}
        -D CMAKE_FIND_ROOT_PATH_MODE_PACKAGE=${CMAKE_FIND_ROOT_PATH_MODE_PACKAGE}
        -D CMAKE_SHARED_LIBRARY_SUFFIX_CXX=_${android_abi}.so
        -D CMAKE_SHARED_MODULE_SUFFIX_CXX=_${android_abi}.so
        -D CMAKE_SHARED_LIBRARY_SUFFIX_C=_${android_abi}.so
        -D CMAKE_SHARED_MODULE_SUFFIX_C=_${android_abi}.so
        -D CMAKE_LIBRARY_OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}/android-build/libs/${android_abi}
        -D ${PROJECT_NAME}-MultiAbiBuild=ON
    )
  endmacro()

  foreach(abi IN LISTS ANDROID_ABIS)
    if (NOT abi STREQUAL ${ANDROID_ABI})
      if (ANDROID_BUILD_ABI_${abi})
        setup_library(${PROJECT_NAME}-${abi} ${abi} ${CMAKE_PREFIX_PATH})
      endif()
    else()
      # For the default abi just use the regular cmake run, to have
      # nice IDE integration and so on
      set(CMAKE_SHARED_MODULE_SUFFIX_CXX "_${ANDROID_ABI}.so")
      set(CMAKE_SHARED_LIBRARY_SUFFIX_CXX "_${ANDROID_ABI}.so")
      set(CMAKE_SHARED_MODULE_SUFFIX_C "_${ANDROID_ABI}.so")
      set(CMAKE_SHARED_LIBRARY_SUFFIX_C "_${ANDROID_ABI}.so")
      set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/android-build/libs/${ANDROID_ABI})
    endif()
  endforeach()
else()
  # unset the variable, just not to issue an unused variable warning
  unset(${PROJECT_NAME}-MultiAbiBuild)
endif()
