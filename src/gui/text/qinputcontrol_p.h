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

#ifndef QINPUTCONTROL_P_H
#define QINPUTCONTROL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qobject.h>
#include <qtguiglobal.h>

QT_BEGIN_NAMESPACE

class QKeyEvent;
class Q_GUI_EXPORT QInputControl : public QObject
{
    Q_OBJECT
public:
    enum Type {
        LineEdit,
        TextEdit
    };

    explicit QInputControl(Type type, QObject *parent = nullptr);

    bool isAcceptableInput(const QKeyEvent *event) const;
    static bool isCommonTextEditShortcut(const QKeyEvent *ke);

protected:
    explicit QInputControl(Type type, QObjectPrivate &dd, QObject *parent = nullptr);

private:
    const Type m_type;
};

QT_END_NAMESPACE

#endif // QINPUTCONTROL_P_H
