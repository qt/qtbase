// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#import <UIKit/UIKit.h>

#include <QtCore/qstring.h>
#include <QtGui/qevent.h>

QT_BEGIN_NAMESPACE

class QIOSInputContext;

QT_END_NAMESPACE

@interface QIOSTextResponder : UIResponder

- (instancetype)initWithInputContext:(QT_PREPEND_NAMESPACE(QIOSInputContext) *)context;

- (void)notifyInputDelegate:(Qt::InputMethodQueries)updatedProperties;
- (BOOL)needsKeyboardReconfigure:(Qt::InputMethodQueries)updatedProperties;
- (void)reset;
- (void)commit;

@end

@interface QIOSTextInputResponder : QIOSTextResponder <UITextInputTraits, UIKeyInput, UITextInput>

- (instancetype)initWithInputContext:(QT_PREPEND_NAMESPACE(QIOSInputContext) *)context;
- (BOOL)needsKeyboardReconfigure:(Qt::InputMethodQueries)updatedProperties;
- (void)reset;
- (void)commit;

- (void)notifyInputDelegate:(Qt::InputMethodQueries)updatedProperties;

@property(readwrite, retain) UIView *inputView;
@property(readwrite, retain) UIView *inputAccessoryView;

// UITextInputTraits
@property(nonatomic) UITextAutocapitalizationType autocapitalizationType;
@property(nonatomic) UITextAutocorrectionType autocorrectionType;
@property(nonatomic) UITextSpellCheckingType spellCheckingType;
@property(nonatomic) BOOL enablesReturnKeyAutomatically;
@property(nonatomic) UIKeyboardAppearance keyboardAppearance;
@property(nonatomic) UIKeyboardType keyboardType;
@property(nonatomic) UIReturnKeyType returnKeyType;
@property(nonatomic, getter=isSecureTextEntry) BOOL secureTextEntry;

// UITextInput
@property(nonatomic, assign) id<UITextInputDelegate> inputDelegate;

@end
