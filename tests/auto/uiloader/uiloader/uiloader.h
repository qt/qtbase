/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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
#include <QList>
#include <QImage>

class uiLoader : public QObject
{
    Q_OBJECT

    public:
        uiLoader(const QString &pathToProgram);

        enum TestResult { TestRunDone, TestConfigError, TestNoConfig };
        TestResult runAutoTests(QString *errorMessage);

    private:
        bool loadConfig(const QString &, QString *errorMessage);
        void initTests();

        void setupFTP();
        void setupLocal();
        void clearDirectory(const QString&);

        void ftpMkDir( QString );
        void ftpGetFiles(QList<QString>&, const QString&,  const QString&);
        void ftpList(const QString&);
        void ftpClearDirectory(const QString&);
        bool ftpUploadFile(const QString&, const QString&);
        void executeTests();

        void createBaseline();
        void downloadBaseline();

        bool compare();

        void diff(const QString&, const QString&, const QString&);
        int imgDiff(const QString fileA, const QString fileB, const QString output);
        QStringList uiFiles() const;
    
        QHash<QString, QString> enginesToTest;

        QString framework;
        QString suite;
        QString output;
        QString ftpUser;
        QString ftpPass;
        QString ftpHost;
        QString ftpBaseDir;
        QString threshold;

        QString errorMsg;

        QList<QString> lsDirList;
        QList<QString> lsNeedBaseline;

        QString configPath;
        
        QString pathToProgram;

    private slots:
        //void ftpAddLsDone( bool );
        void ftpAddLsEntry( const QUrlInfo &urlInfo );
};

#endif
