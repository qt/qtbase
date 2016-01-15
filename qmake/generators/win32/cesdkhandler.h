/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef CE_SDK_HANDLER_INCL
#define CE_SDK_HANDLER_INCL

#include <qstringlist.h>
#include <qdir.h>

QT_BEGIN_NAMESPACE

class CeSdkInfo
{
public:
    CeSdkInfo();
    inline QString name() const { return m_name; }
    inline QString binPath() const { return m_bin; }
    inline QString includePath() const { return m_include; }
    inline QString libPath() const { return m_lib; }
    inline bool isValid() const;
    inline int majorVersion() const { return m_major; }
    inline int minorVersion() const { return m_minor; }
    inline bool isSupported() const { return m_major >= 5; }
private:
    friend class CeSdkHandler;
    QString m_name;
    QString m_bin;
    QString m_include;
    QString m_lib;
    int m_major;
    int m_minor;
};

bool CeSdkInfo::isValid() const
{
    return !m_name.isEmpty() &&
           !m_bin.isEmpty() &&
           !m_include.isEmpty() &&
           !m_lib.isEmpty();
}

class CeSdkHandler
{
public:
    CeSdkHandler();
    bool retrieveAvailableSDKs();
    inline QList<CeSdkInfo> listAll() const { return m_list; }
private:
    void retrieveWEC6n7SDKs();
    void retrieveWEC2013SDKs();
    inline QString fixPaths(const QString &path) const;
    QStringList getMsBuildToolPaths() const;
    QStringList filterMsBuildToolPaths(const QStringList &paths) const;
    bool parseMsBuildFile(QFile *file, CeSdkInfo *info);
    bool retrieveEnvironment(const QStringList &relativePaths,
                             const QStringList &toolPaths,
                             CeSdkInfo *info);
    QList<CeSdkInfo> m_list;
    QString m_vcInstallDir;
};

QT_END_NAMESPACE

#endif
