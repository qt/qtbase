# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(ios_assets LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Test)

qt_add_executable(tst_manual_ios_assets
    main.cpp
)

set_target_properties(tst_manual_ios_assets PROPERTIES
    MACOSX_BUNDLE TRUE
)
target_link_libraries(tst_manual_ios_assets PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Test
)

# Custom Info.plist
set_target_properties(tst_manual_ios_assets
    PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.ios.cmake.plist")

# Custom resources
file(GLOB_RECURSE text_files CONFIGURE_DEPENDS "*.txt")
if(text_files)
    target_sources(tst_manual_ios_assets PRIVATE ${text_files})
    # On iOS the 'Resources' prefix is removed by Xcode because on iOS app bundles are shallow,
    # so the final location of the text file will be
    #    tst_manual_ios_assets.app/textFiles/foo.txt
    # On macOS the location will be
    #    tst_manual_ios_assets.app/Contents/Resources/textFiles/foo.txt
    set_source_files_properties(
        ${text_files}
        PROPERTIES MACOSX_PACKAGE_LOCATION Resources/textFiles)
endif()

# App icons
file(GLOB_RECURSE app_icons CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/appicon/AppIcon*.png")
if(IOS AND app_icons)
    target_sources(tst_manual_ios_assets PRIVATE ${app_icons})
    set_source_files_properties(
        ${app_icons}
        PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif()

# Asset catalog with images
if(IOS)
    enable_language(OBJCXX)
    set(asset_catalog_path "${CMAKE_CURRENT_SOURCE_DIR}/Assets.xcassets")
    target_sources(tst_manual_ios_assets PRIVATE "${asset_catalog_path}")
    set_source_files_properties(
        ${asset_catalog_path}
        PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_sources(tst_manual_ios_assets PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/utils.mm")
endif()

# Set custom launch screen.
# iOS has evolved and provides a few ways to handle this.
# - UILaunchImageFile Info.plist key, introduced in iOS 3.2, supposedly deprecated in iOS 10.
# - UILaunchImages, Info.plist keys,  introduced in iOS 7, deprecated in iOS 13
# - UILaunchStoryboardName, Info.plist key, introduced in iOS 9, not deprecated
# - UILaunchScreen / UILaunchScreens, Info.plist dictionaries, introduced in iOS 14, not
#    deprecated
# The first two expect images, the third one expects a storyboard / .xib file.
# The last ones expect a dictionary of keys to configure the launch screen.
# At the moment, UILaunchStoryboardName represents the lower bound of what Qt supports,
# so use it here.
# Reference info
# https://developer.apple.com/documentation/xcode/specifying-your-apps-launch-screen/
# https://developer.apple.com/library/archive/documentation/General/Reference/InfoPlistKeyReference/Articles/iPhoneOSKeys.html#//apple_ref/doc/uid/TP40009252-SW24
# https://developer.apple.com/documentation/uikit/uilocalnotification/1616660-alertlaunchimage?language=objc
# https://developer.apple.com/documentation/bundleresources/information_property_list/uilaunchimages?language=objc
# https://developer.apple.com/documentation/bundleresources/information_property_list/uilaunchstoryboardname?language=objc
# https://developer.apple.com/documentation/bundleresources/information_property_list/uilaunchscreen?language=objc
# https://forum.qt.io/topic/106251/use-launch-images-in-ios-project/4
# https://codereview.qt-project.org/c/qt/qtdoc/+/100846
if(IOS)
    # Because we're not using the automatically generated Info.plist, it needs to be manually
    # modified to have the UILaunchStoryboardName key.
    set_target_properties(tst_manual_ios_assets PROPERTIES
        QT_IOS_LAUNCH_SCREEN "${CMAKE_CURRENT_SOURCE_DIR}/CustomLaunchScreen.storyboard")
endif()

# Flip to TRUE to debug
if(FALSE)
    target_compile_definitions(tst_manual_ios_assets PRIVATE DEBUG_APP_DATA_LOCATION=1)
endif()
