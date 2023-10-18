TEMPLATE = app
SOURCES += main.cpp
QT += core gui testlib
CONFIG += app_bundle
TARGET = tst_manual_ios_assets

# Custom Info.plist
ios {
    versionAtLeast(QMAKE_XCODE_VERSION, 14.0) {
        plist_path = Info.ios.qmake.xcode.14.3.plist
    } else {
        plist_path = Info.ios.qmake.xcode.13.0.plist
    }
    QMAKE_INFO_PLIST = $$plist_path
}

# Custom resources
textFiles.files = $$files(*.txt)
# On iOS no 'Resources' prefix is needed because iOS app bundles are shallow,
# so the final location of the text file will be
#    tst_manual_ios_assets.app/textFiles/foo.txt
# Specifying a Resources prefix actually causes code signing error for some reason.
# On macOS the location will be
#    tst_manual_ios_assets.app/Contents/Resources/textFiles/foo.txt
ios {
    textFiles.path = textFiles
}
macos {
    textFiles.path = Contents/Resources/textFiles
}
textFiles.files -= CMakeLists.txt
QMAKE_BUNDLE_DATA += textFiles

# Asset catalog with images and app icons
ios {
    # The asset catalog needs to have at least an empty AppIcon.appiconset, otherwise Xcode refuses
    # to compile the asset catalog.
    versionAtLeast(QMAKE_XCODE_VERSION, 14.0) {
        QMAKE_ASSET_CATALOGS += AssetsXcode14.3.xcassets
    } else {
        QMAKE_ASSET_CATALOGS += AssetsXcode13.0.xcassets
    }
    SOURCES += utils.mm
    LIBS += -framework UIKit
}

# Set custom launch screen
ios {
    # Underneath, this uses QMAKE_BUNDLE_DATA, prevents the default launch screen from being set
    # and bundles the custom one.
    QMAKE_IOS_LAUNCH_SCREEN = $$PWD/CustomLaunchScreen.storyboard
}
