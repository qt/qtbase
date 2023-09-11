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
if(IOS)
    if(XCODE_VERSION AND XCODE_VERSION VERSION_LESS "14")
        set(plist_path "${CMAKE_CURRENT_SOURCE_DIR}/Info.ios.cmake.xcode.13.0.plist")
    else()
        set(plist_path "${CMAKE_CURRENT_SOURCE_DIR}/Info.ios.cmake.xcode.14.3.plist")
    endif()
    set_target_properties(tst_manual_ios_assets
        PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${plist_path}")
endif()

# Custom resources
file(GLOB_RECURSE text_files CONFIGURE_DEPENDS "*.txt")
if(text_files)
    list(FILTER text_files EXCLUDE REGEX CMakeLists.txt)
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
# https://developer.apple.com/library/archive/qa/qa1686/_index.html
# https://help.apple.com/xcode/mac/current/#/dev10510b1f7
# https://web.archive.org/web/20180124234409/https://developer.apple.com/ios/human-interface-guidelines/icons-and-images/app-icon/
# https://doc.qt.io/qt-6/ios-platform-notes.html#icons
# No need to copy the icons into the bundle manually when using Xcode 13+.
# - rely on Xcode 13 to copy the needed icon files that are specified in the asset catalog (all the
#   required ones should be specified manually)
# - rely on Xcode 14 to generate the needed icon files based on the 1024x1024 sized image in the
#   asset catalog

# Asset catalog with images and icons.
if(IOS)
    enable_language(OBJCXX)
    if(XCODE_VERSION AND XCODE_VERSION VERSION_LESS "14")
        set(asset_catalog_path "${CMAKE_CURRENT_SOURCE_DIR}/AssetsXcode13.0.xcassets")
    else()
        set(asset_catalog_path "${CMAKE_CURRENT_SOURCE_DIR}/AssetsXcode14.3.xcassets")
    endif()
    target_sources(tst_manual_ios_assets PRIVATE "${asset_catalog_path}")
    set_source_files_properties(
        ${asset_catalog_path}
        PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

    # Make sure asset catalog compilation generates the needed app icons image sizes.
    # This might not be needed in a future Qt version where qt_add_executable might do it
    # automatically. Unclear how to do it cleanly though, because specifying the option when
    # the asset catalog doesn't have an AppIcon set will cause a build failure.
    set_target_properties(tst_manual_ios_assets PROPERTIES
        XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME AppIcon)

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
