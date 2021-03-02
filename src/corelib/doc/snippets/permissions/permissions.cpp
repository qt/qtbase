/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <QtCore/qstring.h>

void takeSelfie() {};

void requestCameraPermissionAndroid()
{
//! [Request camera permission on Android]
     QCoreApplication::requestPermission(QStringLiteral("android.permission.CAMERA"))
             .then([=](QPermission::PermissionResult result) {
        if (result == QPermission::Authorized)
            takeSelfie();
    });
//! [Request camera permission on Android]
}

void requestCameraPermission()
{
//! [Request camera permission]
     QCoreApplication::requestPermission(QPermission::Camera)
             .then([=](QPermission::PermissionResult result) {
        if (result == QPermission::Authorized)
            takeSelfie();
    });
//! [Request camera permission]
}

void requestCameraPermissionSyncAndroid()
{
//! [Request camera permission sync on Android]
    auto future = QCoreApplication::requestPermission(QStringLiteral("android.permission.CAMERA"));
    auto result = future.result(); // blocks and waits for the result to be ready
    if (result == QPermission::Authorized)
        takeSelfie();
//! [Request camera permission sync on Android]
}

void requestCameraPermissionSync()
{
//! [Request camera permission sync]
    auto future = QCoreApplication::requestPermission(QPermission::Camera);
    auto result = future.result(); // blocks and waits for the result to be ready
    if (result == QPermission::Authorized)
        takeSelfie();
//! [Request camera permission sync]
}

void checkCameraPermissionAndroid()
{
//! [Check camera permission on Android]
    QCoreApplication::checkPermission(QStringLiteral("android.permission.CAMERA"))
            .then([=](QPermission::PermissionResult result) {
        if (result == QPermission::Authorized)
            takeSelfie();
    });
//! [Check camera permission on Android]
}

void checkCameraPermission()
{
//! [Check camera permission]
    QCoreApplication::checkPermission(QPermission::Camera)
            .then([=](QPermission::PermissionResult result) {
        if (result == QPermission::Authorized)
            takeSelfie();
    });
//! [Check camera permission]
}

void checkCameraPermissionAndroidSync()
{
//! [Check camera permission sync on Android]
    auto future = QCoreApplication::checkPermission(QStringLiteral("android.permission.CAMERA"));
    // may block and wait for the result to be ready on some platforms
    auto result = future.result();
    if (result == QPermission::Authorized)
        takeSelfie();
//! [Check camera permission sync on Android]
}

void checkCameraPermissionSync()
{
//! [Check camera permission sync]
    auto future = QCoreApplication::checkPermission(QPermission::Camera);
    // may block and wait for the result to be ready on some platforms
    auto result = future.result();
    if (result == QPermission::Authorized)
        takeSelfie();
//! [Check camera permission sync]
}
