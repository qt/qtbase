// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QDesktopServices>
#include <QObject>
#include <QStandardPaths>
#include <QUrl>

namespace src_gui_util_qdesktopservices {

//! [0]
class MyHelpHandler : public QObject
{
    Q_OBJECT
public:
    // ...
public slots:
    void showHelp(const QUrl &url);
};
//! [0]

void wrapper0() {
MyHelpHandler *helpInstance = nullptr;
//! [setUrlHandler]
QDesktopServices::setUrlHandler("help", helpInstance, "showHelp");
//! [setUrlHandler]
} // wrapper


/* comment wrapper 1

//! [1]
mailto:user@foo.com?subject=Test&body=Just a test
//! [1]

*/ // comment wrapper 1


void wrapper1() {
//! [2]
QDesktopServices::openUrl(QUrl("file:///C:/Program Files", QUrl::TolerantMode));
//! [2]
}


/* comment wrapper 2

//! [3]
<key>LSApplicationQueriesSchemes</key>
<array>
    <string>https</string>
</array>
//! [3]

//! [4]
<key>CFBundleURLTypes</key>
<array>
    <dict>
        <key>CFBundleURLSchemes</key>
        <array>
            <string>myapp</string>
        </array>
    </dict>
</array>
//! [4]

*/ // comment wrapper 2

/* comment wrapper 3
//! [7]
<key>com.apple.developer.associated-domains</key>
<array>
    <string>applinks:your.domain.com</string>
</array>
//! [7]

//! [8]
{
    "applinks": {
        "apps": [],
        "details": [{
            "appIDs" : [ "ABCDE12345.com.example.app" ],
            "components": [{
                "/": "/help",
                "?": { "topic": "?*"}
            }]
        }]
    }
}
//! [8]

//! [9]
<intent-filter>
    <action android:name="android.intent.action.VIEW" />
    <category android:name="android.intent.category.DEFAULT" />
    <category android:name="android.intent.category.BROWSABLE" />
    <data android:scheme="https" android:host="your.domain.com" android:port="1337" android:path="/help"/>
</intent-filter>
//! [9]

//! [10]
<intent-filter android:autoVerify="true">
//! [10]

//! [11]
[{
  "relation": ["delegate_permission/common.handle_all_urls"],
  "target": {
    "namespace": "android_app",
    "package_name": "com.example.app",
    "sha256_cert_fingerprints":
    ["14:6D:E9:83:C5:73:06:50:D8:EE:B9:95:2F:34:FC:64:16:A0:83:42:E6:1D:BE:A8:8A:04:96:B2:3F:CF:44:E5"]
  }
}]
//! [11]

*/ // comment wrapper 3

} // src_gui_util_qdesktopservices
