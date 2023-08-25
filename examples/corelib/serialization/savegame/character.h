// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef CHARACTER_H
#define CHARACTER_H

#include <QJsonObject>
#include <QObject>
#include <QString>

QT_FORWARD_DECLARE_CLASS(QTextStream)

//! [0]
class Character
{
    Q_GADGET

public:
    enum ClassType { Warrior, Mage, Archer };
    Q_ENUM(ClassType)

    Character();
    Character(const QString &name, int level, ClassType classType);

    QString name() const;
    void setName(const QString &name);

    int level() const;
    void setLevel(int level);

    ClassType classType() const;
    void setClassType(ClassType classType);

    static Character fromJson(const QJsonObject &json);
    QJsonObject toJson() const;

    void print(QTextStream &s, int indentation = 0) const;

private:
    QString mName;
    int mLevel = 0;
    ClassType mClassType = Warrior;
};
//! [0]

#endif // CHARACTER_H
