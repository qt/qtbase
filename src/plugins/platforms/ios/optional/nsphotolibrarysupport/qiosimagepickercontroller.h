// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QIOSIMAGEPICKERCONTROLLER_H
#define QIOSIMAGEPICKERCONTROLLER_H

#import <UIKit/UIKit.h>

#include "../../qiosfiledialog.h"

@interface QIOSImagePickerController : UIImagePickerController <UIImagePickerControllerDelegate, UINavigationControllerDelegate>
- (instancetype)initWithQIOSFileDialog:(QPlatformFileDialogHelper *)fileDialog;
@end

#endif // QIOSIMAGEPICKERCONTROLLER_H
