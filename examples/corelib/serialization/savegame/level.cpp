// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "level.h"

#include <QJsonArray>
#include <QTextStream>

Level::Level(const QString &name) : mName(name)
{
}

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

//! [0]
void Level::read(const QJsonObject &json)
{
    if (json.contains("name") && json["name"].isString())
        mName = json["name"].toString();

    if (json.contains("npcs") && json["npcs"].isArray()) {
        QJsonArray npcArray = json["npcs"].toArray();
        mNpcs.clear();
        mNpcs.reserve(npcArray.size());
        for (int npcIndex = 0; npcIndex < npcArray.size(); ++npcIndex) {
            QJsonObject npcObject = npcArray[npcIndex].toObject();
            Character npc;
            npc.read(npcObject);
            mNpcs.append(npc);
        }
    }
}
//! [0]

//! [1]
void Level::write(QJsonObject &json) const
{
    json["name"] = mName;
    QJsonArray npcArray;
    for (const Character &npc : mNpcs) {
        QJsonObject npcObject;
        npc.write(npcObject);
        npcArray.append(npcObject);
    }
    json["npcs"] = npcArray;
}
//! [1]

void Level::print(int indentation) const
{
    const QString indent(indentation * 2, ' ');
    QTextStream(stdout) << indent << "Name:\t" << mName << "\n";

    QTextStream(stdout) << indent << "NPCs:\n";
    for (const Character &character : mNpcs)
        character.print(2);
}
