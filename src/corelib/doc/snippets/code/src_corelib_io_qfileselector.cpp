/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

//! [0]
    QString defaultsBasePath = "data/";
    QString defaultsPath = defaultsBasePath + "defaults.conf";
    QString localizedPath = defaultsBasePath
            + QString("%1/defaults.conf").arg(QLocale().name());
    if (QFile::exists(localizedPath))
        defaultsPath = localizedPath;
    QFile defaults(defaultsPath);
//! [0]

//! [1]
    QString defaultsPath = "data/defaults.conf";
#if defined(Q_OS_ANDROID)
    defaultsPath = "data/android/defaults.conf";
#elif defined(Q_OS_IOS)
    defaultsPath = "data/ios/defaults.conf";
#endif
    QFile defaults(defaultsPath);
//! [1]

//! [2]
    QFileSelector selector;
    QFile defaultsFile(selector.select("data/defaults.conf"));
//! [2]

//! [3]
    data/defaults.conf
    data/+android/defaults.conf
    data/+ios/+en_GB/defaults.conf
//! [3]

//! [4]
    images/background.png
    images/+android/+en_GB/background.png
//! [4]

//! [5]
    images/background.png
    images/+linux/background.png
    images/+windows/background.png
    images/+admin/background.png
    images/+admin/+linux/background.png
//! [5]
