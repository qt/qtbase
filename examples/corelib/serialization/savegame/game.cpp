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

//! [0]
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
//! [0]

//! [3]
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
//! [3]

//! [4]
bool Game::saveGame(Game::SaveFormat saveFormat) const
{
    QFile saveFile(saveFormat == Json
        ? QStringLiteral("save.json")
        : QStringLiteral("save.dat"));

    if (!saveFile.open(QIODevice::WriteOnly)) {
        qWarning("Couldn't open save file.");
        return false;
    }

    QJsonObject gameObject;
    write(gameObject);
    saveFile.write(saveFormat == Json
        ? QJsonDocument(gameObject).toJson()
        : QCborValue::fromJsonValue(gameObject).toCbor());

    return true;
}
//! [4]

//! [1]
void Game::read(const QJsonObject &json)
{
    if (json.contains("player") && json["player"].isObject())
        mPlayer.read(json["player"].toObject());

    if (json.contains("levels") && json["levels"].isArray()) {
        QJsonArray levelArray = json["levels"].toArray();
        mLevels.clear();
        mLevels.reserve(levelArray.size());
        for (const QJsonValue &v : levelArray) {
            QJsonObject levelObject = v.toObject();
            Level level;
            level.read(levelObject);
            mLevels.append(level);
        }
    }
}
//! [1]

//! [2]
void Game::write(QJsonObject &json) const
{
    QJsonObject playerObject;
    mPlayer.write(playerObject);
    json["player"] = playerObject;

    QJsonArray levelArray;
    for (const Level &level : mLevels) {
        QJsonObject levelObject;
        level.write(levelObject);
        levelArray.append(levelObject);
    }
    json["levels"] = levelArray;
}
//! [2]

void Game::print(int indentation) const
{
    const QString indent(indentation * 2, ' ');
    QTextStream(stdout) << indent << "Player\n";
    mPlayer.print(indentation + 1);

    QTextStream(stdout) << indent << "Levels\n";
    for (const Level &level : mLevels)
        level.print(indentation + 1);
}
