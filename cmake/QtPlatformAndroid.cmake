# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#
# Self contained Platform Settings for Android
#
# Note: This file is used by the internal builds.
#

#
# Variables:
#   QT_ANDROID_JAR
#       Location of the adroid sdk jar for java code
#   QT_ANDROID_API_USED_FOR_JAVA
#       Android API version for building java code
#

if (NOT DEFINED ANDROID_SDK_ROOT)
    message(FATAL_ERROR "Please provide the location of the Android SDK directory via -DANDROID_SDK_ROOT=<path to Android SDK>")
endif()

if (NOT IS_DIRECTORY "${ANDROID_SDK_ROOT}")
    message(FATAL_ERROR "Could not find ANDROID_SDK_ROOT or path is not a directory: ${ANDROID_SDK_ROOT}")
endif()

# This variable specifies the API level used for building Java code, it can be the same as Qt for
# Android's maximum supported Android version or higher.
if(NOT QT_ANDROID_API_USED_FOR_JAVA)
    set(QT_ANDROID_API_USED_FOR_JAVA "android-35")
endif()

set(jar_location "${ANDROID_SDK_ROOT}/platforms/${QT_ANDROID_API_USED_FOR_JAVA}/android.jar")
if(NOT EXISTS "${jar_location}")
    _qt_internal_detect_latest_android_platform(android_platform_latest)
    if(android_platform_latest)
        message(NOTICE "The default platform SDK ${QT_ANDROID_API_USED_FOR_JAVA} not found, "
                        "using the latest installed ${android_platform_latest} instead.")
        set(QT_ANDROID_API_USED_FOR_JAVA ${android_platform_latest})
    endif()
endif()

set(QT_ANDROID_JAR "${ANDROID_SDK_ROOT}/platforms/${QT_ANDROID_API_USED_FOR_JAVA}/android.jar")
if(NOT EXISTS "${QT_ANDROID_JAR}")
    message(FATAL_ERROR
        "No suitable Android SDK platform found in '${ANDROID_SDK_ROOT}/platforms'."
        " The minimum version required for building Java code is ${QT_ANDROID_API_USED_FOR_JAVA}"
    )
endif()

message(STATUS "Using Android SDK API ${QT_ANDROID_API_USED_FOR_JAVA} from "
               "${ANDROID_SDK_ROOT}/platforms")

# Locate Java
include(UseJava)

# Find JDK 8.0
find_package(Java 1.8 COMPONENTS Development REQUIRED)

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
        QT_ANDROID_APPLICATION_ARGUMENTS
    BRIEF_DOCS
        "This variable can be used to specify command-line arguments to the Android app."
    FULL_DOCS
        "Specifies extra command-line arguments to the Android app using the AndroidManifest.xml with the tag android.app.arguments."
)

define_property(TARGET
    PROPERTY
        QT_ANDROID_DEPLOYMENT_SETTINGS_FILE
    BRIEF_DOCS
        "This variable is used to specify the deployment settings JSON file for androiddeployqt."
    FULL_DOCS
        "This variable points to the path of the deployment settings JSON file, which holds properties required by androiddeployqt to package the Android app."
)

define_property(TARGET
    PROPERTY
        QT_ANDROID_SYSTEM_LIBS_PREFIX
    BRIEF_DOCS
        "This variable is used to specify a path to Qt libraries on the target device in Android."
    FULL_DOCS
        "This variable can be used to provide a custom system library path to use for library loading lookup on Android. This is necessary when using Qt libraries installed outside an app's default native (JNI) library directory."
)

define_property(TARGET
    PROPERTY
        QT_ANDROID_NO_DEPLOY_QT_LIBS
    BRIEF_DOCS
        "This variable is used to control whether Qt libraries should be deployed inside the APK on Android."
    FULL_DOCS
        "This variable can be used to exclude Qt shared libraries from being packaged inside the APK when deploying on Android. Not supported when deploying as Android Application Bundle."
)

# Returns test execution arguments for Android targets
function(qt_internal_android_test_runner_arguments target out_test_runner out_test_arguments)
    set(${out_test_runner} "${QT_HOST_PATH}/${QT${PROJECT_VERSION_MAJOR}_HOST_INFO_BINDIR}/androidtestrunner" PARENT_SCOPE)
    set(deployment_tool "${QT_HOST_PATH}/${QT${PROJECT_VERSION_MAJOR}_HOST_INFO_BINDIR}/androiddeployqt")

    qt_internal_android_get_target_android_build_dir(${target} android_build_dir)
    set(${out_test_arguments}
        "--path" "${android_build_dir}"
        "--adb" "${ANDROID_SDK_ROOT}/platform-tools/adb"
        "--skip-install-root"
        "--make" "\"${CMAKE_COMMAND}\" --build ${CMAKE_BINARY_DIR} --target ${target}_make_apk"
        "--apk" "${android_build_dir}/${target}.apk"
        "--ndk-stack" "${ANDROID_NDK_ROOT}/ndk-stack"
        PARENT_SCOPE
    )
endfunction()
