// Copyright (C) 2020 Harald Meyer.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include "qiosfiledialog.h"

@interface QIOSDocumentPickerController : UIDocumentPickerViewController <UIDocumentPickerDelegate,
                                                                          UINavigationControllerDelegate,
                                                                          UIAdaptivePresentationControllerDelegate>
- (instancetype)initWithQIOSFileDialog:(QIOSFileDialog *)fileDialog;
@end
