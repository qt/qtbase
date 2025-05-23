// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "qcoreapplication.h"
#include "private/qcoreapplication_p.h"
#include <private/qcore_mac_p.h>

QT_BEGIN_NAMESPACE

/*****************************************************************************
  QCoreApplication utility functions
 *****************************************************************************/
QString qAppFileName()
{
    static QString appFileName;
    if (appFileName.isEmpty()) {
        QCFType<CFURLRef> bundleURL(CFBundleCopyExecutableURL(CFBundleGetMainBundle()));
        if (bundleURL) {
            QCFString cfPath(CFURLCopyFileSystemPath(bundleURL, kCFURLPOSIXPathStyle));
            if (cfPath)
                appFileName = cfPath;
        }
    }
    return appFileName;
}

QT_END_NAMESPACE
