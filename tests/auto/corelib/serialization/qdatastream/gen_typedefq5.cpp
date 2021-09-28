/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QDataStream>
#include <QPair>
#include <QFile>
#include <QVariant>
#include <QDebug>

using CustomPair = QPair<int, int>;
QDataStream &operator<<(QDataStream &ds, CustomPair pd)
{ return ds << pd.first << pd.second; }
QDataStream &operator>>(QDataStream &ds, CustomPair &pd)
{ return ds >> pd.first >> pd.second; }
Q_DECLARE_METATYPE(CustomPair)


int main() {
        qRegisterMetaTypeStreamOperators<CustomPair>();
        QFile out("typedef.q5");
        out.open(QIODevice::ReadWrite);
        QDataStream stream(&out);
        stream.setVersion(QDataStream::Qt_5_15);
        CustomPair p {42, 100};
        qDebug() << p.first << p.second;
        stream << QVariant::fromValue(p);
}
