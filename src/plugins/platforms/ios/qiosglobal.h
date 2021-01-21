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

#ifndef QIOSGLOBAL_H
#define QIOSGLOBAL_H

#import <UIKit/UIKit.h>
#include <QtCore/QtCore>

@class QIOSViewController;

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQpaApplication);
Q_DECLARE_LOGGING_CATEGORY(lcQpaInputMethods);
Q_DECLARE_LOGGING_CATEGORY(lcQpaWindow);

#if !defined(QT_NO_DEBUG)
#define qImDebug \
    for (bool qt_category_enabled = lcQpaInputMethods().isDebugEnabled(); qt_category_enabled; qt_category_enabled = false) \
        QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC, lcQpaInputMethods().categoryName()).debug
#else
#define qImDebug QT_NO_QDEBUG_MACRO
#endif

class QPlatformScreen;

bool isQtApplication();

#ifndef Q_OS_TVOS
Qt::ScreenOrientation toQtScreenOrientation(UIDeviceOrientation uiDeviceOrientation);
UIDeviceOrientation fromQtScreenOrientation(Qt::ScreenOrientation qtOrientation);
#endif

int infoPlistValue(NSString* key, int defaultValue);

QT_END_NAMESPACE

@interface UIResponder (QtFirstResponder)
+ (id)currentFirstResponder;
@end

QT_BEGIN_NAMESPACE

class FirstResponderCandidate : public QScopedValueRollback<UIResponder *>
{
public:
     FirstResponderCandidate(UIResponder *);
     static UIResponder *currentCandidate() { return s_firstResponderCandidate; }

private:
    static UIResponder *s_firstResponderCandidate;
};

QT_END_NAMESPACE

#endif // QIOSGLOBAL_H
