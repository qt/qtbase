// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef LEVEL_H
#define LEVEL_H

#include <QJsonObject>
#include <QList>

#include "character.h"

//! [0]
class Level
{
public:
    Level() = default;
    Level(const QString &name);

    QString name() const;

    QList<Character> npcs() const;
    void setNpcs(const QList<Character> &npcs);

    void read(const QJsonObject &json);
    void write(QJsonObject &json) const;

    void print(int indentation = 0) const;
private:
    QString mName;
    QList<Character> mNpcs;
};
//! [0]

#endif // LEVEL_H
