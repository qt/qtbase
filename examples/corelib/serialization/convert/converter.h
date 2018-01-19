/****************************************************************************
**
** Copyright (C) 2018 Intel Corporation.
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

#ifndef CONVERTER_H
#define CONVERTER_H

#include <QIODevice>
#include <QPair>
#include <QVariant>
#include <QVector>

class VariantOrderedMap : public QVector<QPair<QVariant, QVariant>>
{
public:
    VariantOrderedMap() = default;
    VariantOrderedMap(const QVariantMap &map)
    {
        reserve(map.size());
        for (auto it = map.begin(); it != map.end(); ++it)
            append({it.key(), it.value()});
    }
};
using Map = VariantOrderedMap;
Q_DECLARE_METATYPE(Map)

class Converter
{
protected:
    Converter();

public:
    static Converter *null;

    enum Direction {
        In = 1, Out = 2, InOut = 3
    };

    enum Option {
        SupportsArbitraryMapKeys = 0x01
    };
    Q_DECLARE_FLAGS(Options, Option)

    virtual ~Converter() = 0;

    virtual QString name() = 0;
    virtual Direction directions() = 0;
    virtual Options outputOptions() = 0;
    virtual const char *optionsHelp() = 0;
    virtual bool probeFile(QIODevice *f) = 0;
    virtual QVariant loadFile(QIODevice *f, Converter *&outputConverter) = 0;
    virtual void saveFile(QIODevice *f, const QVariant &contents, const QStringList &options) = 0;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Converter::Options)

#endif // CONVERTER_H
