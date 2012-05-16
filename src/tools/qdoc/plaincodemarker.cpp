/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "plaincodemarker.h"

QT_BEGIN_NAMESPACE

PlainCodeMarker::PlainCodeMarker()
{
}

PlainCodeMarker::~PlainCodeMarker()
{
}

bool PlainCodeMarker::recognizeCode( const QString& /* code */ )
{
    return true;
}

bool PlainCodeMarker::recognizeExtension( const QString& /* ext */ )
{
    return true;
}

bool PlainCodeMarker::recognizeLanguage( const QString& /* lang */ )
{
    return false;
}

Atom::Type PlainCodeMarker::atomType() const
{
    return Atom::Code;
}

QString PlainCodeMarker::plainName( const Node * /* node */ )
{
    return QString();
}

QString PlainCodeMarker::plainFullName(const Node * /* node */, const Node * /* relative */)
{
    return QString();
}

QString PlainCodeMarker::markedUpCode( const QString& code,
                                       const Node * /* relative */,
                                       const Location & /* location */ )
{
    return protect( code );
}

QString PlainCodeMarker::markedUpSynopsis( const Node * /* node */,
                                           const Node * /* relative */,
                                           SynopsisStyle /* style */ )
{
    return "foo";
}

QString PlainCodeMarker::markedUpName( const Node * /* node */ )
{
    return QString();
}

QString PlainCodeMarker::markedUpFullName( const Node * /* node */,
                                           const Node * /* relative */ )
{
    return QString();
}

QString PlainCodeMarker::markedUpEnumValue(const QString & /* enumValue */,
                                           const Node * /* relative */)
{
    return QString();
}

QString PlainCodeMarker::markedUpIncludes( const QStringList& /* includes */ )
{
    return QString();
}

QString PlainCodeMarker::functionBeginRegExp( const QString& /* funcName */ )
{
    return QString();
}

QString PlainCodeMarker::functionEndRegExp( const QString& /* funcName */ )
{
    return QString();
}

QList<Section> PlainCodeMarker::sections(const InnerNode * /* innerNode */,
                                         SynopsisStyle /* style */,
                                         Status /* status */)
{
    return QList<Section>();
}

QT_END_NAMESPACE
