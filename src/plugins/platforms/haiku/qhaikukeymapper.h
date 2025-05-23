// Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Tobias Koenig <tobias.koenig@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QHAIKUKEYMAPPER_H
#define QHAIKUKEYMAPPER_H

#include <qnamespace.h>

#include <InterfaceDefs.h>

QT_BEGIN_NAMESPACE

class QHaikuKeyMapper
{
public:
    QHaikuKeyMapper();

    static uint32 translateKeyCode(uint32 key, bool numlockActive);
};

QT_END_NAMESPACE

#endif
