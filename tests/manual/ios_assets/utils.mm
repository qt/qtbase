// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtCore/QString>

#ifdef Q_OS_IOS
#include <UIKit/UIKit.h>
#endif

bool imageExistsInAssetCatalog(QString imageName) {
#ifdef Q_OS_IOS
    UIImage* image = [UIImage imageNamed:imageName.toNSString()];
    return image != nil;
#else
    // Don't fail the test when building on platforms other than iOS.
    return true;
#endif
}
