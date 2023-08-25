// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "level.h"

#include <QJsonArray>
#include <QTextStream>

Level::Level(const QString &name) : mName(name) { }

QString Level::name() const
{
    return mName;
}

QList<Character> Level::npcs() const
{
    return mNpcs;
}

void Level::setNpcs(const QList<Character> &npcs)
{
    mNpcs = npcs;
}

//! [fromJson]
Level Level::fromJson(const QJsonObject &json)
{
    Level result;

    if (const QJsonValue v = json["name"]; v.isString())
        result.mName = v.toString();

    if (const QJsonValue v = json["npcs"]; v.isArray()) {
        const QJsonArray npcs = v.toArray();
        result.mNpcs.reserve(npcs.size());
        for (const QJsonValue &npc : npcs)
            result.mNpcs.append(Character::fromJson(npc.toObject()));
    }

    return result;
}
//! [fromJson]

//! [toJson]
QJsonObject Level::toJson() const
{
    QJsonObject json;
    json["name"] = mName;
    QJsonArray npcArray;
    for (const Character &npc : mNpcs)
        npcArray.append(npc.toJson());
    json["npcs"] = npcArray;
    return json;
}
//! [toJson]

void Level::print(QTextStream &s, int indentation) const
{
    const QString indent(indentation * 2, ' ');

    s << indent << "Name:\t" << mName << "\n" << indent << "NPCs:\n";
    for (const Character &character : mNpcs)
        character.print(s, indentation + 1);
}
