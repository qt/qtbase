/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QWIDGETITEMDATA_P_H
#define QWIDGETITEMDATA_P_H

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <QtCore/qdatastream.h>

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

QT_BEGIN_NAMESPACE

class QWidgetItemData
{
public:
    inline QWidgetItemData() : role(-1) {}
    inline QWidgetItemData(int r, const QVariant &v) : role(r), value(v) {}
    int role;
    QVariant value;
    inline bool operator==(const QWidgetItemData &other) const { return role == other.role && value == other.value; }
};
Q_DECLARE_TYPEINFO(QWidgetItemData, Q_MOVABLE_TYPE);

#ifndef QT_NO_DATASTREAM

inline QDataStream &operator>>(QDataStream &in, QWidgetItemData &data)
{
    in >> data.role;
    in >> data.value;
    return in;
}

inline QDataStream &operator<<(QDataStream &out, const QWidgetItemData &data)
{
    out << data.role;
    out << data.value;
    return out;
}

#endif // QT_NO_DATASTREAM

QT_END_NAMESPACE

#endif // QWIDGETITEMDATA_P_H
