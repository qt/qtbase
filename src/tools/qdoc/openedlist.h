/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

/*
  openedlist.h
*/

#ifndef OPENEDLIST_H
#define OPENEDLIST_H

#include <qstring.h>

#include "location.h"

QT_BEGIN_NAMESPACE

class OpenedList
{
    Q_DECLARE_TR_FUNCTIONS(QDoc::OpenedList)

public:
    enum Style { Bullet, Tag, Value, Numeric, UpperAlpha, LowerAlpha,
                 UpperRoman, LowerRoman };

    OpenedList()
        : sty( Bullet ), ini( 1 ), nex( 0 ) { }
    OpenedList( Style style );
    OpenedList( const Location& location, const QString& hint );

    void next() { nex++; }

    bool isStarted() const { return nex >= ini; }
    Style style() const { return sty; }
    QString styleString() const;
    int number() const { return nex; }
    QString numberString() const;
    QString prefix() const { return pref; }
    QString suffix() const { return suff; }

private:
    static QString toAlpha( int n );
    static int fromAlpha( const QString& str );
    static QString toRoman( int n );
    static int fromRoman( const QString& str );

    Style sty;
    int ini;
    int nex;
    QString pref;
    QString suff;
};
Q_DECLARE_TYPEINFO(OpenedList, Q_MOVABLE_TYPE);

QT_END_NAMESPACE

#endif
