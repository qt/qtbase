/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QURL_P_H
#define QURL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists for the convenience of
// qurl*.cpp This header file may change from version to version without
// notice, or even be removed.
//
// We mean it.
//

#include "qurl.h"

QT_BEGIN_NAMESPACE

struct QUrlErrorInfo {
    inline QUrlErrorInfo() : _source(0), _message(0), _expected(0), _found(0)
    { }

    const char *_source;
    const char *_message;
    char _expected;
    char _found;

    inline void setParams(const char *source, const char *message, char expected, char found)
    {
        _source = source;
        _message = message;
        _expected = expected;
        _found = found;
    }
};

struct QUrlParseData
{
    const char *scheme;
    int schemeLength;

    const char *userInfo;
    int userInfoDelimIndex;
    int userInfoLength;

    const char *host;
    int hostLength;
    int port;

    const char *path;
    int pathLength;
    const char *query;
    int queryLength;
    const char *fragment;
    int fragmentLength;

    QUrlErrorInfo *errorInfo;
};

// in qurlrecode.cpp
extern Q_AUTOTEST_EXPORT int qt_urlRecode(QString &appendTo, const QChar *begin, const QChar *end,
                                          QUrl::ComponentFormattingOptions encoding, const ushort *tableModifications);

// in qurlidna.cpp
enum AceOperation { ToAceOnly, NormalizeAce };
extern QString qt_ACE_do(const QString &domain, AceOperation op);
extern Q_AUTOTEST_EXPORT void qt_nameprep(QString *source, int from);
extern Q_AUTOTEST_EXPORT bool qt_check_std3rules(const QChar *uc, int len);
extern Q_AUTOTEST_EXPORT void qt_punycodeEncoder(const QChar *s, int ucLength, QString *output);
extern Q_AUTOTEST_EXPORT QString qt_punycodeDecoder(const QString &pc);

// in qurlparser.cpp
extern bool qt_urlParse(const char *ptr, QUrlParseData &parseData);
extern bool qt_isValidUrlIP(const char *ptr);

QT_END_NAMESPACE

#endif // QURL_P_H
