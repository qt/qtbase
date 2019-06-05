#
# Platform Settings for Android
#

if (NOT DEFINED ANDROID_SDK_ROOT)
    message(FATAL_ERROR "ANDROID_SDK_ROOT is not defined")
endif()

if (NOT IS_DIRECTORY "${ANDROID_SDK_ROOT}")
    message(FATAL_ERROR "Could not find ANDROID_SDK_ROOT or path is not a directory: ${ANDROID_SDK_ROOT}")
endif()

# Minimum recommend android SDK api version
set(qt_android_api_version "android-21")

# Locate android.jar
set(android_jar "${ANDROID_SDK_ROOT}/platforms/${qt_android_api_version}/android.jar")
if(NOT EXISTS "${android_jar}")
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
        set(qt_android_api_version ${android_platform_latest})
        set(android_jar "${ANDROID_SDK_ROOT}/platforms/${qt_android_api_version}/android.jar")
    endif()
endif()

if(NOT EXISTS "${android_jar}")
    message(FATAL_ERROR "No suitable Android SDK platform found. Minimum version is ${qt_android_api_version}")
endif()

message(STATUS "Using Android SDK API ${qt_android_api_version} from ${ANDROID_SDK_ROOT}/platforms")

# Locate Java
include(UseJava)

# Find JDK 8.0
find_package(Java 1.8 COMPONENTS Development REQUIRED)
