/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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
