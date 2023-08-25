// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef LEVEL_H
#define LEVEL_H

#include "character.h"

#include <QJsonObject>
#include <QList>

QT_FORWARD_DECLARE_CLASS(QTextStream)

//! [0]
class Level
{
public:
    Level() = default;
    explicit Level(const QString &name);

    QString name() const;

    QList<Character> npcs() const;
    void setNpcs(const QList<Character> &npcs);

    static Level fromJson(const QJsonObject &json);
    QJsonObject toJson() const;

    void print(QTextStream &s, int indentation = 0) const;

private:
    QString mName;
    QList<Character> mNpcs;
};
//! [0]

#endif // LEVEL_H
