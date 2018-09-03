/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
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
