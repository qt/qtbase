/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2013 Ivan Komissarov.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#ifndef QKEYSEQUENCEEDIT_P_H
#define QKEYSEQUENCEEDIT_P_H

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

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include "qkeysequenceedit.h"

#include <private/qwidget_p.h>
#include <private/qkeysequence_p.h>

QT_REQUIRE_CONFIG(keysequenceedit);

QT_BEGIN_NAMESPACE

class QLineEdit;

class QKeySequenceEditPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QKeySequenceEdit)
public:
    void init();
    int translateModifiers(Qt::KeyboardModifiers state, const QString &text);
    void resetState();
    void finishEditing();

    QLineEdit *lineEdit;
    QKeySequence keySequence;
    int keyNum;
    int key[QKeySequencePrivate::MaxKeyCount];
    int prevKey;
    int releaseTimer;
};

QT_END_NAMESPACE

#endif // QKEYSEQUENCEEDIT_P_H
