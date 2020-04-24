#
# Self contained Platform Settings for Android
#
# Note: This file is used both by the internal and public builds.
#

#
# Public variables:
#   QT_ANDROID_JAR
#       Location of the adroid sdk jar for java code
#   QT_ANDROID_APIVERSION
#       Android API version
#   QT_ANDROID_SDK_BUILD_TOOLS_VERSION
#       Detected Android sdk build tools version
#
# Public functions:
#
#   qt_android_generate_deployment_settings()
#       Generate the deployment settings json file for a cmake target.
#

if (NOT DEFINED ANDROID_SDK_ROOT)
    message(FATAL_ERROR "Please provide the location of the Android SDK directory via -DANDROID_SDK_ROOT=<path to Adndroid SDK>")
endif()

if (NOT IS_DIRECTORY "${ANDROID_SDK_ROOT}")
    message(FATAL_ERROR "Could not find ANDROID_SDK_ROOT or path is not a directory: ${ANDROID_SDK_ROOT}")
endif()

# Get the Android SDK jar for an API version other than the one specified with
# QT_ANDROID_API_VERSION.
function(qt_get_android_sdk_jar_for_api api out_jar_location)
    set(jar_location "${ANDROID_SDK_ROOT}/platforms/${api}/android.jar")
    if (NOT EXISTS "${jar_location}")
        message(WARNING "Could not locate Android SDK jar for api '${api}', defaulting to ${QT_ANDROID_API_VERSION}")
        set(${out_jar_location} ${QT_ANDROID_JAR} PARENT_SCOPE)
    else()
        set(${out_jar_location} ${jar_location} PARENT_SCOPE)
    endif()
endfunction()

# Minimum recommend android SDK api version
set(QT_ANDROID_API_VERSION "android-21")

# Locate android.jar
set(QT_ANDROID_JAR "${ANDROID_SDK_ROOT}/platforms/${QT_ANDROID_API_VERSION}/android.jar")
if(NOT EXISTS "${QT_ANDROID_JAR}")
    # Locate the highest available platform
    file(GLOB android_platforms
        LIST_DIRECTORIES true
        RELATIVE "${ANDROID_SDK_ROOT}/platforms"
        "${ANDROID_SDK_ROOT}/platforms/*")
    # If list is not empty
    if(android_platforms)
        list(SORT android_platforms)
        list(REVERSE android_platforms)
        list(GET android_platforms 0 android_platform_latest)
        set(QT_ANDROID_API_VERSION ${android_platform_latest})
        set(QT_ANDROID_JAR "${ANDROID_SDK_ROOT}/platforms/${QT_ANDROID_API_VERSION}/android.jar")
    endif()
endif()

if(NOT EXISTS "${QT_ANDROID_JAR}")
    message(FATAL_ERROR "No suitable Android SDK platform found. Minimum version is ${QT_ANDROID_API_VERSION}")
endif()

message(STATUS "Using Android SDK API ${QT_ANDROID_API_VERSION} from ${ANDROID_SDK_ROOT}/platforms")

# Locate Java
include(UseJava)

# Find JDK 8.0
find_package(Java 1.8 COMPONENTS Development REQUIRED)

# Locate newest android sdk build tools
if (NOT QT_ANDROID_SDK_BUILD_TOOLS_VERSION)
    file(GLOB android_build_tools
        LIST_DIRECTORIES true
        RELATIVE "${ANDROID_SDK_ROOT}/build-tools"
        "${ANDROID_SDK_ROOT}/build-tools/*")
    if (NOT android_build_tools)
        message(FATAL_ERROR "Could not locate Android SDK build tools under \"${ANDROID_SDK}/build-tools\"")
    endif()
    list(SORT android_build_tools)
    list(REVERSE android_build_tools)
    list(GET android_build_tools 0 android_build_tools_latest)
    set(QT_ANDROID_SDK_BUILD_TOOLS_VERSION ${android_build_tools_latest})
endif()

# Ensure we are using the shared version of libc++
if(NOT ANDROID_STL STREQUAL c++_shared)
    message(FATAL_ERROR "The Qt libraries on Android only supports the shared library configuration of stl. Please use -DANDROID_STL=\"c++_shared\" as configuration argument.")
endif()

# Target properties required for android deploy tool
define_property(TARGET
    PROPERTY
        QT_ANDROID_DEPLOYMENT_DEPENDENCIES
    BRIEF_DOCS
        "Specify additional plugins that need to be deployed with the current android application"
    FULL_DOCS
        "By default, androiddeployqt will detect the dependencies of your application. But since run-time usage of plugins cannot be detected, there could be false positives, as your application will depend on any plugins that are potential dependencies. If you want to minimize the size of your APK, it's possible to override the automatic detection using the ANDROID_DEPLOYMENT_DEPENDENCIES variable. This should contain a list of all Qt files which need to be included, with paths relative to the Qt install root. Note that only the Qt files specified here will be included. Failing to include the correct files can result in crashes. It's also important to make sure the files are listed in the correct loading order. This variable provides a way to override the automatic detection entirely, so if a library is listed before its dependencies, it will fail to load on some devices."
)

define_property(TARGET
    PROPERTY
        QT_ANDROID_EXTRA_LIBS
    BRIEF_DOCS
        "A list of external libraries that will be copied into your application's library folder and loaded on start-up."
    FULL_DOCS
    "A list of external libraries that will be copied into your application's library folder and loaded on start-up. This can be used, for instance, to enable OpenSSL in your application. Simply set the paths to the required libssl.so and libcrypto.so libraries here and OpenSSL should be enabled automatically."
)

define_property(TARGET
    PROPERTY
        QT_ANDROID_EXTRA_PLUGINS
    BRIEF_DOCS
        "This variable can be used to specify different resources that your project has to bundle but cannot be delivered through the assets system, such as qml plugins."
    FULL_DOCS
        "This variable can be used to specify different resources that your project has to bundle but cannot be delivered through the assets system, such as qml plugins. When using this variable, androiddeployqt will make sure everything is packaged and deployed properly."
)

define_property(TARGET
    PROPERTY
        QT_ANDROID_PACKAGE_SOURCE_DIR
    BRIEF_DOCS
        "This variable can be used to specify a directory where additions and modifications can be made to the default Android package template."
    FULL_DOCS
        "This variable can be used to specify a directory where additions and modifications can be made to the default Android package template. The androiddeployqt tool will copy the application template from Qt into the build directory, and then it will copy the contents of the ANDROID_PACKAGE_SOURCE_DIR on top of this, overwriting any existing files. The update step where parts of the source files are modified automatically to reflect your other settings is then run on the resulting merged package. If you, for instance, want to make a custom AndroidManifest.xml for your application, then place this directly into the folder specified in this variable. You can also add custom Java files in ANDROID_PACKAGE_SOURCE_DIR/src."
)

define_property(TARGET
    PROPERTY
        QT_ANDROID_DEPLOYMENT_SETTINGS_FILE
    BRIEF_DOCS
        " "
    FULL_DOCS
        " "
)

# Generate deployment tool json
function(qt_android_generate_deployment_settings target)
    # Information extracted from mkspecs/features/android/android_deployment_settings.prf
    if (NOT TARGET ${target})
        message(SEND_ERROR "${target} is not a cmake target")
        return()
    endif()

    get_target_property(target_type ${target} TYPE)

    if (NOT "${target_type}" STREQUAL "MODULE_LIBRARY")
        message(SEND_ERROR "QT_ANDROID_GENERATE_DEPLOYMENT_SETTINGS only works on Module targets")
        return()
    endif()

    get_target_property(target_source_dir ${target} SOURCE_DIR)
    get_target_property(target_binary_dir ${target} BINARY_DIR)
    get_target_property(target_output_name ${target} OUTPUT_NAME)
    if (NOT target_output_name)
        set(target_output_name ${target})
    endif()
    set(deploy_file "${target_binary_dir}/android-lib${target_output_name}.so-deployment-settings.json")

    set(file_contents "{\n")
    # content begin
    string(APPEND file_contents
        "   \"description\": \"This file is generated by cmake to be read by androiddeployqt and should not be modified by hand.\",\n")

    # Host Qt Android  install path
    if (NOT QT_BUILDING_QT)
        set(file_check "${Qt6_DIR}/plugins/platforms/android/libqtforandroid_${CMAKE_ANDROID_ARCH_ABI}.so")
        if (NOT EXISTS ${file_check})
            message(SEND_ERROR "Detected Qt installation does not contain libqtforandroid.so. This is most likely due to the installation not being a build of Qt for Android. Please update your settings.")
            return()
        endif()
        set(qt_android_install_dir ${Qt6_Dir})
    else()
        # Building from source, use the same install prefix
        set(qt_android_install_dir ${CMAKE_INSTALL_PREFIX})
    endif()

    file(TO_NATIVE_PATH "${qt_android_install_dir}" qt_android_install_dir_native)
    string(APPEND file_contents
        "   \"qt\": \"${qt_android_install_dir_native}\",\n")

    # Android SDK path
    file(TO_NATIVE_PATH "${ANDROID_SDK_ROOT}" android_sdk_root_native)
    string(APPEND file_contents
        "   \"sdk\": \"${android_sdk_root_native}\",\n")

    # Android SDK Build Tools Revision
    string(APPEND file_contents
        "   \"sdkBuildToolsRevision\": \"${QT_ANDROID_SDK_BUILD_TOOLS_VERSION}\",\n")

    # Android NDK
    file(TO_NATIVE_PATH "${ANDROID_NDK}" android_ndk_root_native)
    string(APPEND file_contents
        "   \"ndk\": \"${android_ndk_root_native}\",\n")

    # Setup LLVM toolchain
    string(APPEND file_contents
        "   \"toolchain-prefix\": \"llvm\",\n")
    string(APPEND file_contents
        "   \"tool-prefix\": \"llvm\",\n")
    string(APPEND file_contents
        "   \"useLLVM\": true,\n")

    # NDK Toolchain Version
    string(APPEND file_contents
        "   \"toolchain-version\": \"${CMAKE_ANDROID_NDK_TOOLCHAIN_VERSION}\",\n")

    # NDK Host
    string(APPEND file_contents
        "   \"ndk-host\": \"${ANDROID_NDK_HOST_SYSTEM_NAME}\",\n")

    if (CMAKE_ANDROID_ARCH_ABI STREQUAL "x86")
        set(arch_value "i686-linux-android")
    elseif (CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64")
        set(arch_value "x86_64-linux-android")
    elseif (CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
        set(arch_value "aarch64-linux-android")
    else()
        set(arch_value "arm-linux-androideabi")
    endif()

    # Architecture
    string(APPEND file_contents
        "   \"architectures\": { \"${CMAKE_ANDROID_ARCH_ABI}\" : \"${arch_value}\" },\n")

    # deployment dependencies
    get_target_property(android_deployment_dependencies
        ${target} QT_ANDROID_DEPLOYMENT_DEPENDENCIES)
    if (android_deployment_dependencies)
        list(JOIN android_deployment_dependencies "," android_deployment_dependencies)
        string(APPEND file_contents
            "   \"deployment-dependencies\": \"${android_deployment_dependencies}\",\n")
    endif()

    # Extra plugins
    get_target_property(android_extra_plugins
        ${target} QT_ANDROID_EXTRA_PLUGINS)
    if (android_extra_plugins)
        list(JOIN android_extra_plugins "," android_extra_plugins)
        string(APPEND file_contents
            "   \"android-extra-plugins\": \"${android_extra_plugins}\",\n")
    endif()

    # Extra libs
    get_target_property(android_extra_libs
        ${target} QT_ANDROID_EXTRA_LIBS)
    if (android_extra_libs)
        list(JOIN android_extra_libs "," android_extra_libs)
        string(APPEND file_contents
            "   \"android-extra-libs\": \"${android_extra_libs}\",\n")
    endif()

    # package source dir
    get_target_property(android_package_source_dir
        ${target} QT_ANDROID_PACKAGE_SOURCE_DIR)
    if (android_package_source_dir)
        file(TO_NATIVE_PATH "${android_package_source_dir}" android_package_source_dir_native)
        string(APPEND file_contents
            "   \"android-package-source-directory\": \"${android_package_source_dir_native}\",\n")
endif()

    #TODO: ANDROID_VERSION_NAME, doesn't seem to be used?

    #TODO: ANDROID_VERSION_CODE, doesn't seem to be used?

    get_target_property(qml_import_path ${target} QT_QML_IMPORT_PATH)
    if (qml_import_path)
        file(TO_NATIVE_PATH "${qml_import_path}" qml_import_path_native)
        string(APPEND file_contents
            "   \"qml-import-path\": \"${qml_import_path_native}\",\n")
    endif()

    get_target_property(qml_root_path ${target} QT_QML_ROOT_PATH)
    if(NOT qml_root_path)
        set(qml_root_path "${target_source_dir}")
    endif()
    file(TO_NATIVE_PATH "${qml_root_path}" qml_root_path_native)
    string(APPEND file_contents
        "   \"qml-root-path\": \"${qml_root_path_native}\",\n")

    # App binary
    string(APPEND file_contents
        "   \"application-binary\": \"${target_output_name}\",\n")

    # Override qmlimportscanner binary path
    set(qml_importscanner_binary_path "${QT_HOST_PATH}/bin/qmlimportscanner")
    if (WIN32)
        string(APPEND qml_importscanner_binary_path ".exe")
    endif()
    file(TO_NATIVE_PATH "${qml_importscanner_binary_path}" qml_importscanner_binary_path_native)
    string(APPEND file_contents
        "   \"qml-importscanner-binary\" : \"${qml_importscanner_binary_path_native}\",\n")

    # Last item in json file

    # base location of stdlibc++, will be suffixed by androiddeploy qt
    set(android_ndk_stdlib_base_path
        "${ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/lib/"
    )
    string(APPEND file_contents
        "   \"stdcpp-path\": \"${android_ndk_stdlib_base_path}\"\n")

    # content end
    string(APPEND file_contents "}\n")

    file(WRITE ${deploy_file} ${file_contents})

    set_target_properties(${target}
        PROPERTIES
            QT_ANDROID_DEPLOYMENT_SETTINGS_FILE ${deploy_file}
    )
endfunction()

function(qt_android_apply_arch_suffix target)
    get_target_property(target_type ${target} TYPE)
    if (target_type STREQUAL "SHARED_LIBRARY" OR target_type STREQUAL "MODULE_LIBRARY")
        set_property(TARGET "${target}" PROPERTY SUFFIX "_${CMAKE_ANDROID_ARCH_ABI}.so")
    elseif (target_type STREQUAL "STATIC_LIBRARY")
        set_property(TARGET "${target}" PROPERTY SUFFIX "_${CMAKE_ANDROID_ARCH_ABI}.a")
    endif()
endfunction()

# Add custom target to package the APK
function(qt_android_add_apk_target target)
    get_target_property(deployment_file ${target} QT_ANDROID_DEPLOYMENT_SETTINGS_FILE)
    if (NOT deployment_file)
        message(FATAL_ERROR "Target ${target} is not a valid android executable target\n")
    endif()

    set(deployment_tool "${QT_HOST_PATH}/bin/androiddeployqt")
    set(apk_dir "$<TARGET_PROPERTY:${target},BINARY_DIR>/android-build")
    add_custom_target(${target}_prepare_apk_dir
        DEPENDS ${target}
        COMMAND ${CMAKE_COMMAND}
            -E copy $<TARGET_FILE:${target}>
            "${apk_dir}/libs/${CMAKE_ANDROID_ARCH_ABI}/$<TARGET_FILE_NAME:${target}>"
        COMMENT "Copying ${target} binarty to apk folder"
    )

    add_custom_target(${target}_make_apk
        DEPENDS ${target}_prepare_apk_dir
        COMMAND  ${deployment_tool}
            --input ${deployment_file}
            --output ${apk_dir}
        COMMENT "Creating APK for ${target}"
    )
endfunction()


# Add a test for Android which will be run by the android test runner tool
function(qt_android_add_test target)

    set(deployment_tool "${QT_HOST_PATH}/bin/androiddeployqt")
    set(test_runner "${QT_HOST_PATH}/bin/androidtestrunner")

    get_target_property(deployment_file ${target} QT_ANDROID_DEPLOYMENT_SETTINGS_FILE)
    if (NOT deployment_file)
        message(FATAL_ERROR "Target ${target} is not a valid android executable target\n")
    endif()

    set(target_binary_dir "$<TARGET_PROPERTY:${target},BINARY_DIR>")
    set(apk_dir "${target_binary_dir}/android-build")

    add_test(NAME "${target}"
        COMMAND "${test_runner}"
            --androiddeployqt "${deployment_tool} --input ${deployment_file}"
            --adb "${ANDROID_SDK_ROOT}/platform-tools/adb"
            --path "${apk_dir}"
            --skip-install-root
            --make "${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ${target}_make_apk"
            --apk "${apk_dir}/${target}.apk"
            --verbose
    )
endfunction()
