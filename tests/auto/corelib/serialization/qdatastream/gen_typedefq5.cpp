// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


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
