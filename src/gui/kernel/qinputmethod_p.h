/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QINPUTMETHOD_P_H
#define QINPUTMETHOD_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include <qinputmethod.h>
#include <private/qobject_p.h>
#include <QtCore/QWeakPointer>
#include <QTransform>
#include <qpa/qplatforminputcontext.h>
#include <qpa/qplatformintegration.h>
#include <private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

class QInputMethodPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QInputMethod)

public:
    inline QInputMethodPrivate() : testContext(nullptr)
    {}
    QPlatformInputContext *platformInputContext() const
    {
        return testContext ? testContext : QGuiApplicationPrivate::platformIntegration()->inputContext();
    }
    static inline QInputMethodPrivate *get(QInputMethod *inputMethod)
    {
        return inputMethod->d_func();
    }

    void _q_connectFocusObject();
    void _q_checkFocusObject(QObject *object);
    static bool objectAcceptsInputMethod(QObject *object);

    QTransform inputItemTransform;
    QRectF inputRectangle;
    QPlatformInputContext *testContext;
};

QT_END_NAMESPACE

#endif
