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
#   QT_ANDROID_NDK_STDLIB_PATH
#       Detected path to the c++ stl lib shared library
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


# Location of stdlibc++
set(QT_ANDROID_NDK_STDLIB_PATH "${ANDROID_NDK}/sources/cxx-stl/llvm-libc++/libs/${ANDROID_ABI}/libc++_shared.so")

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

    file(WRITE ${deploy_file} "{\n")
    # content begin
    file(APPEND ${deploy_file}
        "   \"description\": \"This file is generated by cmake to be read by androiddeployqt and should not be modified by hand.\",\n")

    # Host Qt Android  install path
    if (NOT QT_BUILDING_QT)
        set(file_check "${Qt6_DIR}/plugins/platforms/android/libqtforandroid.so")
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
    file(APPEND ${deploy_file}
        "   \"qt\": \"${qt_android_install_dir_native}\",\n")

    # Android SDK path
    file(TO_NATIVE_PATH "${ANDROID_SDK_ROOT}" android_sdk_root_native)
    file(APPEND ${deploy_file}
        "   \"sdk\": \"${android_sdk_root_native}\",\n")

    # Android SDK Build Tools Revision
    file(APPEND ${deploy_file}
        "   \"sdkBuildToolsRevision\": \"${QT_ANDROID_SDK_BUILD_TOOLS_VERSION}\",\n")

    # Android NDK
    file(TO_NATIVE_PATH "${ANDROID_NDK}" android_ndk_root_native)
    file(APPEND ${deploy_file}
        "   \"ndk\": \"${android_ndk_root_native}\",\n")

    # Setup LLVM toolchain
    file(APPEND ${deploy_file}
        "   \"toolchain-prefix\": \"llvm\",\n")
    file(APPEND ${deploy_file}
        "   \"tool-prefix\": \"llvm\",\n")
    file(APPEND ${deploy_file}
        "   \"useLLVM\": true,\n")

    # NDK Toolchain Version
    file(APPEND ${deploy_file}
        "   \"toolchain-version\": \"${CMAKE_ANDROID_NDK_TOOLCHAIN_VERSION}\",\n")

    # NDK Host
    file(APPEND ${deploy_file}
        "   \"ndk-host\": \"${ANDROID_NDK_HOST_SYSTEM_NAME}\",\n")

    # Architecture
    file(APPEND ${deploy_file}
        "   \"target-architecture\": \"${CMAKE_ANDROID_ARCH_ABI}\",\n")

    # deployment dependencies
    get_target_property(android_deployment_dependencies
        ${target} QT_ANDROID_DEPLOYMENT_DEPENDENCIES)
    if (android_deployment_dependencies)
        list(JOIN android_deployment_dependencies "," android_deployment_dependencies)
        file(APPEND ${deploy_file}
            "   \"deployment-dependencies\": \"${android_deployment_dependencies}\",\n")
    endif()

    # Extra plugins
    get_target_property(android_extra_plugins
        ${target} QT_ANDROID_EXTRA_PLUGINS)
    if (android_extra_plugins)
        list(JOIN android_extra_plugins "," android_extra_plugins)
        file(APPEND ${deploy_file}
            "   \"android-extra-plugins\": \"${android_extra_plugins}\",\n")
    endif()

    # Extra libs
    get_target_property(android_extra_libs
        ${target} QT_ANDROID_EXTRA_LIBS)
    if (android_extra_libs)
        list(JOIN android_extra_libs "," android_extra_libs)
        file(APPEND ${deploy_file}
            "   \"android-extra-libs\": \"${android_extra_libs}\",\n")
    endif()

    # package source dir
    get_target_property(android_package_source_dir
        ${target} QT_ANDROID_PACKAGE_SOURCE_DIR)
    if (android_package_source_dir)
        file(TO_NATIVE_PATH "${android_package_source_dir}" android_package_source_dir_native)
        file(APPEND ${deploy_file}
            "   \"android-package-source-directory\": \"${android_package_source_dir_native}\",\n")
endif()

    #TODO: ANDROID_VERSION_NAME, doesn't seem to be used?

    #TODO: ANDROID_VERSION_CODE, doesn't seem to be used?

    get_target_property(qml_import_path ${target} QT_QML_IMPORT_PATH)
    if (qml_import_path)
        file(TO_NATIVE_PATH "${qml_import_path}" qml_import_path_native)
        file(APPEND ${deploy_file}
            "   \"qml-import-path\": \"${qml_import_path_native}\",\n")
    endif()

    get_target_property(qml_root_path ${target} QT_QML_ROOT_PATH)
    if(NOT qml_root_path)
        set(qml_root_path "${target_source_dir}")
    endif()
    file(TO_NATIVE_PATH "${qml_root_path}" qml_root_path_native)
    file(APPEND ${deploy_file}
        "   \"qml-root-path\": \"${qml_root_path_native}\",\n")

    # App binary
    file(TO_NATIVE_PATH "${target_binary_dir}/lib${target_output_name}.so" target_binary_dir_native)
    file(APPEND ${deploy_file}
        "   \"application-binary\": \"${target_binary_dir_native}\",\n")

    # Lats item in json file

    file(APPEND ${deploy_file}
        "   \"stdcpp-path\": \"${QT_ANDROID_NDK_STDLIB_PATH}\"\n")

    # content end
    file(APPEND ${deploy_file} "}\n")

    set_target_properties(${target}
        PROPERTIES
            QT_ANDROID_DEPLOYMENT_SETTINGS_FILE ${deploy_file}
    )
endfunction()
