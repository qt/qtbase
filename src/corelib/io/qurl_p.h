/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2012 Intel Corporation.
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
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
        Fragment = 0x80,
        FullUrl = 0xff
    };

    enum ErrorCode {
        // the high byte of the error code matches the Section
        InvalidSchemeError = Scheme << 8,
        SchemeEmptyError,

        InvalidUserNameError = UserName << 8,

        InvalidPasswordError = Password << 8,

        InvalidRegNameError = Host << 8,
        InvalidIPv4AddressError,
        InvalidIPv6AddressError,
        InvalidIPvFutureError,
        HostMissingEndBracket,

        InvalidPortError = Port << 8,
        PortEmptyError,

        InvalidPathError = Path << 8,

        InvalidQueryError = Query << 8,

        InvalidFragmentError = Fragment << 8,

        // the following two cases are only possible in combination
        // with presence/absence of the authority and scheme. See validityError().
        AuthorityPresentAndPathIsRelative = Authority << 8 | Path << 8 | 0x10000,
        RelativeUrlPathContainsColonBeforeSlash = Scheme << 8 | Authority << 8 | Path << 8 | 0x10000,

        NoError = 0
    };

    QUrlPrivate();
    QUrlPrivate(const QUrlPrivate &copy);

    void parse(const QString &url, QUrl::ParsingMode parsingMode);
    bool isEmpty() const
    { return sectionIsPresent == 0 && port == -1 && path.isEmpty(); }
    ErrorCode validityError() const;

    // no QString scheme() const;
    void appendAuthority(QString &appendTo, QUrl::FormattingOptions options, Section appendingTo) const;
    void appendUserInfo(QString &appendTo, QUrl::FormattingOptions options, Section appendingTo) const;
    void appendUserName(QString &appendTo, QUrl::FormattingOptions options) const;
    void appendPassword(QString &appendTo, QUrl::FormattingOptions options) const;
    void appendHost(QString &appendTo, QUrl::FormattingOptions options) const;
    void appendPath(QString &appendTo, QUrl::FormattingOptions options, Section appendingTo) const;
    void appendQuery(QString &appendTo, QUrl::FormattingOptions options, Section appendingTo) const;
    void appendFragment(QString &appendTo, QUrl::FormattingOptions options) const;

    // the "end" parameters are like STL iterators: they point to one past the last valid element
    bool setScheme(const QString &value, int len);
    bool setAuthority(const QString &auth, int from, int end, QUrl::ParsingMode mode);
    void setUserInfo(const QString &userInfo, int from, int end);
    void setUserName(const QString &value, int from, int end);
    void setPassword(const QString &value, int from, int end);
    bool setHost(const QString &value, int from, int end, QUrl::ParsingMode mode);
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
                                          QUrl::ComponentFormattingOptions encoding, const ushort *tableModifications = 0);

// in qurlidna.cpp
enum AceOperation { ToAceOnly, NormalizeAce };
extern QString qt_ACE_do(const QString &domain, AceOperation op);
extern Q_AUTOTEST_EXPORT void qt_nameprep(QString *source, int from);
extern Q_AUTOTEST_EXPORT bool qt_check_std3rules(const QChar *uc, int len);
extern Q_AUTOTEST_EXPORT void qt_punycodeEncoder(const QChar *s, int ucLength, QString *output);
extern Q_AUTOTEST_EXPORT QString qt_punycodeDecoder(const QString &pc);
extern Q_AUTOTEST_EXPORT QString qt_urlRecodeByteArray(const QByteArray &ba);

QT_END_NAMESPACE

#endif // QURL_P_H
