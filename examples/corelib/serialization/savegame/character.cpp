// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "character.h"

#include <QMetaEnum>
#include <QTextStream>

Character::Character() :
    mLevel(0),
    mClassType(Warrior) {
}

Character::Character(const QString &name,
                     int level,
                     Character::ClassType classType) :
    mName(name),
    mLevel(level),
    mClassType(classType)
{
}

QString Character::name() const
{
    return mName;
}

void Character::setName(const QString &name)
{
    mName = name;
}

int Character::level() const
{
    return mLevel;
}

void Character::setLevel(int level)
{
    mLevel = level;
}

Character::ClassType Character::classType() const
{
    return mClassType;
}

void Character::setClassType(Character::ClassType classType)
{
    mClassType = classType;
}

//! [0]
void Character::read(const QJsonObject &json)
{
    if (json.contains("name") && json["name"].isString())
        mName = json["name"].toString();

    if (json.contains("level") && json["level"].isDouble())
        mLevel = json["level"].toInt();

    if (json.contains("classType") && json["classType"].isDouble())
        mClassType = ClassType(json["classType"].toInt());
}
//! [0]

//! [1]
void Character::write(QJsonObject &json) const
{
    json["name"] = mName;
    json["level"] = mLevel;
    json["classType"] = mClassType;
}
//! [1]

void Character::print(int indentation) const
{
    const QString indent(indentation * 2, ' ');
    QTextStream(stdout) << indent << "Name:\t" << mName << "\n";
    QTextStream(stdout) << indent << "Level:\t" << mLevel << "\n";

    QString className = QMetaEnum::fromType<ClassType>().valueToKey(mClassType);
    QTextStream(stdout) << indent << "Class:\t" << className << "\n";
}
