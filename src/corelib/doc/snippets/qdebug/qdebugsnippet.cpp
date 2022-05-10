// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtGui>
#include <QtDebug>

class Coordinate : public QObject
{
public:
    int myX, myY;

    int x() const { return myX; };
    int y() const { return myY; };
};

//! [0]
QDebug operator<<(QDebug debug, const Coordinate &c)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << '(' << c.x() << ", " << c.y() << ')';

    return debug;
}
//! [0]

int main(int argv, char **args)
{
    Coordinate coordinate;
    coordinate.myX = 10;
    coordinate.myY = 44;

//! [1]
    qDebug() << "Date:" << QDate::currentDate();
    qDebug() << "Types:" << QString("String") << QChar('x') << QRect(0, 10, 50, 40);
    qDebug() << "Custom coordinate type:" << coordinate;
//! [1]
}
