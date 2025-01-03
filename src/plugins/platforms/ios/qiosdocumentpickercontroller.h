// Copyright (C) 2020 Harald Meyer.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include "qiosfiledialog.h"

@interface QIOSDocumentPickerController : UIDocumentPickerViewController <UIDocumentPickerDelegate,
                                                                          UINavigationControllerDelegate,
                                                                          UIAdaptivePresentationControllerDelegate>
- (instancetype)initWithQIOSFileDialog:(QIOSFileDialog *)fileDialog;
@end
