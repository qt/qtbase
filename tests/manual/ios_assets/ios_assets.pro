TEMPLATE = app
SOURCES += main.cpp
QT += core gui testlib
CONFIG += app_bundle
TARGET = tst_manual_ios_assets

# Custom Info.plist
ios {
    QMAKE_INFO_PLIST = Info.ios.qmake.plist
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
QMAKE_BUNDLE_DATA += textFiles

# App icons
ios {
    ios_icon.files = $$files($$PWD/appicon/AppIcon*.png)
    QMAKE_BUNDLE_DATA += ios_icon
}

# Asset catalog with images
ios {
    # The asset catalog needs to have an empty AppIcon.appiconset, otherwise Xcode refuses
    # to compile the asset catalog.
    QMAKE_ASSET_CATALOGS += Assets.xcassets
    SOURCES += utils.mm
    LIBS += -framework UIKit
}

# Set custom launch screen
ios {
    # Underneath, this uses QMAKE_BUNDLE_DATA, prevents the default launch screen from being set
    # and bundles the custom one.
    QMAKE_IOS_LAUNCH_SCREEN = $$PWD/CustomLaunchScreen.storyboard
}
