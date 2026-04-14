// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qcoreapplication.h"
#include "private/qcoreapplication_p.h"
#include <private/qcore_mac_p.h>

#include <qfileinfo.h>

QT_BEGIN_NAMESPACE

/*****************************************************************************
  QCoreApplication utility functions
 *****************************************************************************/
QString qAppFileName()
{
    // QCoreApplication::applicationFilePath() expects a canonical path
    QString appFileName;
    QCFType<CFURLRef> bundleURL(CFBundleCopyExecutableURL(CFBundleGetMainBundle()));
    if (bundleURL) {
        QCFString cfPath(CFURLCopyFileSystemPath(bundleURL, kCFURLPOSIXPathStyle));
        if (cfPath)
            appFileName = QFileInfo(cfPath).canonicalFilePath();
    }
    return appFileName;
}

QT_END_NAMESPACE
