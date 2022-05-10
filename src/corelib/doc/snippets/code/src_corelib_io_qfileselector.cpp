// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
