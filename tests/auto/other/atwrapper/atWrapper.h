/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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
#ifndef ATWRAPPER_H
#define ATWRAPPER_H

#include <QHash>
#include <QString>
#include <QUrlInfo>
#include <QColor>

class atWrapper : public QObject
{
    Q_OBJECT

    public:
        atWrapper();
        bool runAutoTests();

    private:
        bool executeTests();
        bool initTests(bool *haveBaseline);
        bool compare();
        void createBaseline();
        bool loadConfig( QString );
        void compareDirs( QString, QString );
        bool diff( QString, QString, QString );
        void downloadBaseline();
        void uploadFailed( QString, QString, QByteArray );
        bool ftpMkDir( QString );
        void ftpRmDir( QString );
        bool setupFTP();
        void uploadDiff( QString, QString, QString );

        QHash<QString, QString> enginesToTest;
        QString framework;
        QString suite;
        QString output;
        QString size;
        QString ftpUser;
        QString ftpPass;
        QString ftpHost;
        QString ftpBaseDir;
        QList<QString> rmDirList;
        QList<QString> mgetDirList;
        QString configPath;
        QString fillColor;

    private slots:
        void ftpRmDirAddToList( const QUrlInfo &urlInfo );
        void ftpRmDirDone( bool );
        void ftpMgetAddToList( const QUrlInfo &urlInfo );
        void ftpMgetDone( bool );
};

#endif
