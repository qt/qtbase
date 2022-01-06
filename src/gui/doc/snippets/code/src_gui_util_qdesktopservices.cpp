/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/
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


void wrapper3() {
//! [6]
QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
    "/data/organization/application";
//! [6]
} // wrapper3


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
