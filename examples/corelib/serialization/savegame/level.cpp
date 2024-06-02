/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

    s << indent << "Name:\t" << mName << "\n"
      << indent << "NPCs:\n";
    for (const Character &character : mNpcs)
        character.print(s, indentation + 1);
}
