/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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
