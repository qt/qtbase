// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "game.h"

#include <QCborMap>
#include <QCborValue>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRandomGenerator>
#include <QTextStream>

Character Game::player() const
{
    return mPlayer;
}

QList<Level> Game::levels() const
{
    return mLevels;
}

//! [newGame]
void Game::newGame()
{
    mPlayer = Character();
    mPlayer.setName(QStringLiteral("Hero"));
    mPlayer.setClassType(Character::Archer);
    mPlayer.setLevel(QRandomGenerator::global()->bounded(15, 21));

    mLevels.clear();
    mLevels.reserve(2);

    Level village(QStringLiteral("Village"));
    QList<Character> villageNpcs;
    villageNpcs.reserve(2);
    villageNpcs.append(Character(QStringLiteral("Barry the Blacksmith"),
                                 QRandomGenerator::global()->bounded(8, 11),
                                 Character::Warrior));
    villageNpcs.append(Character(QStringLiteral("Terry the Trader"),
                                 QRandomGenerator::global()->bounded(6, 8),
                                 Character::Warrior));
    village.setNpcs(villageNpcs);
    mLevels.append(village);

    Level dungeon(QStringLiteral("Dungeon"));
    QList<Character> dungeonNpcs;
    dungeonNpcs.reserve(3);
    dungeonNpcs.append(Character(QStringLiteral("Eric the Evil"),
                                 QRandomGenerator::global()->bounded(18, 26),
                                 Character::Mage));
    dungeonNpcs.append(Character(QStringLiteral("Eric's Left Minion"),
                                 QRandomGenerator::global()->bounded(5, 7),
                                 Character::Warrior));
    dungeonNpcs.append(Character(QStringLiteral("Eric's Right Minion"),
                                 QRandomGenerator::global()->bounded(4, 9),
                                 Character::Warrior));
    dungeon.setNpcs(dungeonNpcs);
    mLevels.append(dungeon);
}
//! [newGame]

//! [loadGame]
bool Game::loadGame(Game::SaveFormat saveFormat)
{
    QFile loadFile(saveFormat == Json
        ? QStringLiteral("save.json")
        : QStringLiteral("save.dat"));

    if (!loadFile.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open save file.");
        return false;
    }

    QByteArray saveData = loadFile.readAll();

    QJsonDocument loadDoc(saveFormat == Json
        ? QJsonDocument::fromJson(saveData)
        : QJsonDocument(QCborValue::fromCbor(saveData).toMap().toJsonObject()));

    read(loadDoc.object());

    QTextStream(stdout) << "Loaded save for "
                        << loadDoc["player"]["name"].toString()
                        << " using "
                        << (saveFormat != Json ? "CBOR" : "JSON") << "...\n";
    return true;
}
//! [loadGame]

//! [saveGame]
bool Game::saveGame(Game::SaveFormat saveFormat) const
{
    QFile saveFile(saveFormat == Json
        ? QStringLiteral("save.json")
        : QStringLiteral("save.dat"));

    if (!saveFile.open(QIODevice::WriteOnly)) {
        qWarning("Couldn't open save file.");
        return false;
    }

    QJsonObject gameObject = toJson();
    saveFile.write(saveFormat == Json
        ? QJsonDocument(gameObject).toJson()
        : QCborValue::fromJsonValue(gameObject).toCbor());

    return true;
}
//! [saveGame]

//! [read]
void Game::read(const QJsonObject &json)
{
    if (const QJsonValue v = json["player"]; v.isObject())
        mPlayer = Character::fromJson(v.toObject());

    if (const QJsonValue v = json["levels"]; v.isArray()) {
        const QJsonArray levels = v.toArray();
        mLevels.clear();
        mLevels.reserve(levels.size());
        for (const QJsonValue &level : levels)
            mLevels.append(Level::fromJson(level.toObject()));
    }
}
//! [read]

//! [toJson]
QJsonObject Game::toJson() const
{
    QJsonObject json;
    json["player"] = mPlayer.toJson();

    QJsonArray levels;
    for (const Level &level : mLevels)
        levels.append(level.toJson());
    json["levels"] = levels;
    return json;
}
//! [toJson]

void Game::print(QTextStream &s, int indentation) const
{
    const QString indent(indentation * 2, ' ');
    s << indent << "Player\n";
    mPlayer.print(s, indentation + 1);

    s << indent << "Levels\n";
    for (const Level &level : mLevels)
        level.print(s, indentation + 1);
}
