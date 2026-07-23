// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSAPPBUNDLEINFO_H
#define QOHOSAPPBUNDLEINFO_H

#include <QtOhosAppKit/qtohosappkitglobal.h>

QT_BEGIN_NAMESPACE

namespace QtOhosAppKit {

class Q_OHOSAPPKIT_EXPORT BundleInfo
{
public:
    virtual ~BundleInfo();

    virtual int versionCode() const = 0;

protected:
    BundleInfo();

private:
    Q_DISABLE_COPY(BundleInfo)
};

}

QT_END_NAMESPACE

#endif
