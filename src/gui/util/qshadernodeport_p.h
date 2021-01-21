/****************************************************************************
**
** Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
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

#ifndef QSHADERNODEPORT_P_H
#define QSHADERNODEPORT_P_H

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

#include <QtCore/qstring.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

class QShaderNodePort
{
public:
    enum Direction : char {
        Input,
        Output
    };

    Q_GUI_EXPORT QShaderNodePort() noexcept;

    QShaderNodePort::Direction direction;
    QString name;
};

Q_GUI_EXPORT bool operator==(const QShaderNodePort &lhs, const QShaderNodePort &rhs) noexcept;

inline bool operator!=(const QShaderNodePort &lhs, const QShaderNodePort &rhs) noexcept
{
    return !(lhs == rhs);
}

Q_DECLARE_TYPEINFO(QShaderNodePort, Q_MOVABLE_TYPE);

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QShaderNodePort)

#endif // QSHADERNODEPORT_P_H
