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

class QUrlPrivate
{
public:
    enum Section {
        Scheme = 0x01,
        UserName = 0x02,
        Password = 0x04,
        UserInfo = UserName | Password,
        Host = 0x08,
        Port = 0x10,
        Authority = UserInfo | Host | Port,
        Path = 0x20,
        Hierarchy = Authority | Path,
        Query = 0x40,
        Fragment = 0x80
    };

    enum ErrorCode {
        InvalidSchemeError = 0x000,
        SchemeEmptyError,

        InvalidRegNameError = 0x800,
        InvalidIPv4AddressError,
        InvalidIPv6AddressError,
        InvalidIPvFutureError,
        HostMissingEndBracket,

        InvalidPortError = 0x1000,
        PortEmptyError,

        PathContainsColonBeforeSlash = 0x2000,

        NoError = 0xffff
    };

    QUrlPrivate();
    QUrlPrivate(const QUrlPrivate &copy);

    void parse(const QString &url);
    void clear();

    // no QString scheme() const;
    void appendAuthority(QString &appendTo, QUrl::FormattingOptions options) const;
    void appendUserInfo(QString &appendTo, QUrl::FormattingOptions options) const;
    void appendUserName(QString &appendTo, QUrl::FormattingOptions options) const;
    void appendPassword(QString &appendTo, QUrl::FormattingOptions options) const;
    void appendHost(QString &appendTo, QUrl::FormattingOptions options) const;
    void appendPath(QString &appendTo, QUrl::FormattingOptions options) const;
    void appendQuery(QString &appendTo, QUrl::FormattingOptions options) const;
    void appendFragment(QString &appendTo, QUrl::FormattingOptions options) const;

    // the "end" parameters are like STL iterators: they point to one past the last valid element
    bool setScheme(const QString &value, int len, bool decoded = false);
    bool setAuthority(const QString &auth, int from, int end);
    void setUserInfo(const QString &userInfo, int from, int end);
    void setUserName(const QString &value, int from, int end);
    void setPassword(const QString &value, int from, int end);
    bool setHost(const QString &value, int from, int end, bool maybePercentEncoded = true);
    void setPath(const QString &value, int from, int end);
    void setQuery(const QString &value, int from, int end);
    void setFragment(const QString &value, int from, int end);

    inline bool hasScheme() const { return sectionIsPresent & Scheme; }
    inline bool hasAuthority() const { return sectionIsPresent & Authority; }
    inline bool hasUserInfo() const { return sectionIsPresent & UserInfo; }
    inline bool hasUserName() const { return sectionIsPresent & UserName; }
    inline bool hasPassword() const { return sectionIsPresent & Password; }
    inline bool hasHost() const { return sectionIsPresent & Host; }
    inline bool hasPort() const { return port != -1; }
    inline bool hasPath() const { return !path.isEmpty(); }
    inline bool hasQuery() const { return sectionIsPresent & Query; }
    inline bool hasFragment() const { return sectionIsPresent & Fragment; }

    QString mergePaths(const QString &relativePath) const;

    QAtomicInt ref;
    int port;

    QString scheme;
    QString userName;
    QString password;
    QString host;
    QString path;
    QString query;
    QString fragment;

    ushort errorCode;
    ushort errorSupplement;

    // not used for:
    //  - Port (port == -1 means absence)
    //  - Path (there's no path delimiter, so we optimize its use out of existence)
    // Schemes are never supposed to be empty, but we keep the flag anyway
    uchar sectionIsPresent;

    // UserName, Password, Path, Query, and Fragment never contain errors in TolerantMode.
    // Those flags are set only by the strict parser.
    uchar sectionHasError;
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

QT_END_NAMESPACE

#endif // QURL_P_H
