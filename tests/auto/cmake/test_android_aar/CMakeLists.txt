# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(test_android_aar)

find_package(Qt6 COMPONENTS Core Gui REQUIRED)

qt6_add_executable(test_aar main.cpp)

# To support opening the project in QtCreator
if($CACHE{QT_USE_TARGET_ANDROID_BUILD_DIR})
    set(TARGET_ANDROID_BUILD_DIR ${CMAKE_BINARY_DIR}/android-build-test_aar)
else()
    set(TARGET_ANDROID_BUILD_DIR ${CMAKE_BINARY_DIR}/android-build)
endif()

# Add common libs for available ABIs that should be present in the aar package in a list
unset(aar_content_filepaths_to_verify)
foreach(abi IN LISTS QT_ANDROID_ABIS)
   list(APPEND aar_content_filepaths_to_verify "jni/${abi}/libQt6Core_${abi}.so")
   list(APPEND aar_content_filepaths_to_verify "jni/${abi}/libQt6Gui_${abi}.so")
   list(APPEND aar_content_filepaths_to_verify
            "jni/${abi}/libplugins_platforms_qtforandroid_${abi}.so")
   list(APPEND aar_content_filepaths_to_verify "jni/${abi}/libtest_aar_${abi}.so")
endforeach()

# Add a few ABI independent file that should be present in the aar package in a list
list(APPEND aar_content_filepaths_to_verify "libs/Qt6Android.jar")
list(APPEND aar_content_filepaths_to_verify "res/xml/qtprovider_paths.xml")
list(APPEND aar_content_filepaths_to_verify "AndroidManifest.xml")
list(JOIN aar_content_filepaths_to_verify "|"  grep_pattern)

# The overall number of lines we should expect to be filtered by grep regex
list(LENGTH aar_content_filepaths_to_verify expected_file_count)

set(zipinfo_command "zipinfo -1 ${TARGET_ANDROID_BUILD_DIR}/test_aar.aar")

# Runs zipinfo on the aar package, greps and outputs the number of matching lines
# and finally compares the expected number of lines with the output
add_custom_target(verify_aar
    bash -c "${zipinfo_command} \
    | grep -Ecx '${grep_pattern}' \
    | grep -xq ${expected_file_count} \
    || { echo 'Error: The aar package is missing at least one file.'; exit 1; };"
    COMMENT "Verifying aar package content"
    VERBATIM
)

# Build aar package before verification
add_dependencies(verify_aar aar)

target_link_libraries(test_aar PRIVATE Qt::Core Qt::Gui)
