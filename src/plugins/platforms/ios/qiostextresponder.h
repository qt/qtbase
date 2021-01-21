/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#import <UIKit/UIKit.h>

#include <QtCore/qstring.h>
#include <QtGui/qevent.h>

QT_BEGIN_NAMESPACE

class QIOSInputContext;

QT_END_NAMESPACE

@interface QIOSTextInputResponder : UIResponder <UITextInputTraits, UIKeyInput, UITextInput>

- (instancetype)initWithInputContext:(QT_PREPEND_NAMESPACE(QIOSInputContext) *)context;
- (BOOL)needsKeyboardReconfigure:(Qt::InputMethodQueries)updatedProperties;

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
