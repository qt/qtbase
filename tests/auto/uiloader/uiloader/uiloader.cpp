/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include "uiloader.h"

#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QSettings>
#include <QtCore/QCoreApplication>

#include <QtTest/QSignalSpy>
#include <QTest>

#include <QString>
#include <QHash>
#include <QFile>
#include <QFtp>
#include <QObject>
#include <QHostInfo>
#include <QWidget>
#include <QImage>

#include <QLibraryInfo>


/*
 * Our own QVERIFY since the one from QTest can't be used in non-void functions.
 * Just pass the desired return value as third argument.
 */

#define QVERIFY3(statement, description, returnValue) \
do {\
    if (statement) {\
        if (!QTest::qVerify(true, #statement, (description), __FILE__, __LINE__))\
            return returnValue;\
    } else {\
        if (!QTest::qVerify(false, #statement, (description), __FILE__, __LINE__))\
            return returnValue;\
    }\
} while (0)



uiLoader::uiLoader(const QString &_pathToProgram)
    : pathToProgram(_pathToProgram)
{
  //   initTests();
}




/*
 * Load the configuration file for your machine.
 * Return true if everything was loaded, else false.
 *
 * If the hostname is 'kayak', the config file should be 'kayak.ini':
 *
 *  [General]
 *  ftpBaseDir=/arthurtest
 *  ftpHost=wartburg
 *  ftpPass=anonymouspass
 *  ftpUser=anonymous
 *  output=testresults
 *
 *  [engines]
 *  1\engine=uic
 *  size=1
 */

bool uiLoader::loadConfig(const QString &filePath, QString *errorMessage)
{
    qDebug() << " ========== Loading config file " << filePath;
    configPath = filePath;

    // If there is no config file, dont proceed;
    QSettings settings( filePath, QSettings::IniFormat, this );

    // all keys available?
    QStringList keyList;
    keyList << QLatin1String("output") << QLatin1String("ftpUser") << QLatin1String("ftpPass") << QLatin1String("ftpHost") << QLatin1String("ftpBaseDir");
    for (int i = 0; i < keyList.size(); ++i) {
        const QString currentKey = keyList.at(i);
        if (!settings.contains(currentKey)) {
            *errorMessage = QString::fromLatin1("Config file '%1' does not contain the required key '%2'.").arg(filePath, currentKey);
            return false;
        }

        qDebug() << "\t\t(I)" << currentKey << "\t" << settings.value(currentKey).toString();
    }

    const int size = settings.beginReadArray(QLatin1String("engines"));
    if (!size) {
        *errorMessage = QString::fromLatin1("Config file '%1' does not contain the necessary section engines.").arg(filePath);
        return false;
    }

    // get the values
    for ( int i = 0; i < size; ++i ) {
        settings.setArrayIndex(i);
        qDebug() << "\t\t(I)" << "engine" << "\t" << settings.value( "engine" ).toString();
        enginesToTest.insert(settings.value(QLatin1String("engine")).toString(), QLatin1String("Info here please :p"));
    }
    settings.endArray();

    output = settings.value(QLatin1String("output")).toString();
    output += QDir::separator() + QLibraryInfo::buildKey() + QDir::separator() + QString( qVersion() );
    ftpUser = settings.value( QLatin1String("ftpUser") ).toString();
    ftpPass = settings.value( QLatin1String("ftpPass") ).toString();
    ftpHost = settings.value( QLatin1String("ftpHost") ).toString();
    ftpBaseDir = settings.value( QLatin1String("ftpBaseDir") ).toString() + QDir::separator() + QHostInfo::localHostName().split( QLatin1Char('.')).first();
    threshold = settings.value( QLatin1String("threshold") ).toString();

    qDebug() << "\t(I) Values adapted:";
    qDebug() << "\t\t(I)" << "ftpBaseDir" << "\t" << ftpBaseDir;
    qDebug() << "\t\t(I)" << "output" << "\t" << output;

    return true;
}

/*
 * Upload testresults to the server in order to create the new baseline.
 */

void uiLoader::createBaseline()
{
    // can't use ftpUploadFile() here
    qDebug() << " ========== Uploading baseline of only the latest test values ";

    QFtp ftp;
    ftp.connectToHost( ftpHost );
    ftp.login( ftpUser, ftpPass );
    ftp.cd( ftpBaseDir );

    QDir dir( output );

    // Upload all the latest test results to the FTP server's baseline directory.
    QHashIterator<QString, QString> i(enginesToTest);
    while ( i.hasNext() ) {
        i.next();

        dir.cd( i.key() );
        ftp.cd( i.key() + ".baseline" );

        dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
        dir.setNameFilters( QStringList() << "*.png" );
        QFileInfoList list = dir.entryInfoList();

        dir.cd( ".." );

        for (int n = 0; n < list.size(); n++) {
            QFileInfo fileInfo = list.at( n );
            QFile file( QString( output ) + "/" + i.key() + "/" + fileInfo.fileName() );

            errorMsg = "could not open file " + fileInfo.fileName();
            QVERIFY2( file.open(QIODevice::ReadOnly), qPrintable(errorMsg));

            QByteArray fileData = file.readAll();
            file.close();

            ftp.put( fileData, fileInfo.fileName(), QFtp::Binary );
            qDebug() << "\t(I) Uploading:" << fileInfo.fileName() << "with file size" << fileData.size();
        }

        ftp.cd( ".." );
    }

    ftp.close();

    while ( ftp.hasPendingCommands() )
        QCoreApplication::instance()->processEvents();
}



/*
 * Download baseline from server in order to compare results.
 */

void uiLoader::downloadBaseline()
{
    qDebug() << " ========== Downloading baseline...";

    QHashIterator<QString, QString> i(enginesToTest);
    while ( i.hasNext() ) {
        i.next();
        QString engineName = i.key();

        QString dirWithFiles = ftpBaseDir + '/' + engineName + ".baseline";
        QString ftpDir = ftpBaseDir + '/' + engineName + ".baseline";
        QString saveToDir = QDir::currentPath() + '/' + output + '/' + engineName + ".baseline";

        ftpList(dirWithFiles);

        QList<QString> filesToDownload(lsDirList);
        ftpGetFiles(filesToDownload, ftpDir, saveToDir);
    }
}



/*
 * Enter the dir pathDir local and remove all files (not recursive!)
 */

void uiLoader::clearDirectory(const QString& pathDir)
{
    qDebug() << "\t(I) Clearing directory local: " << pathDir;

    QDir dir(pathDir);
    dir.setFilter(QDir::Files);
    QStringList list = dir.entryList();

    for (int n = 0; n < list.size(); n++) {
        QString filePath = pathDir + "/" + list.at(n);
        QFile file(filePath);

        errorMsg = "could not remove file " + filePath;
        QVERIFY2( file.remove(), qPrintable(errorMsg));
    }

}



/*
 * Setup the local environment.
 */

void uiLoader::setupLocal()
{
    qDebug( " ========== Setting up local environment" );

    QDir dir;

    errorMsg = "could not create path " + output;
    QVERIFY2( dir.mkpath(output), qPrintable(errorMsg) );

    QHashIterator<QString, QString> j(enginesToTest);
    while ( j.hasNext() ) {
        j.next();

        QString engineName = j.key();
        QString engineDir = output + '/' + engineName;

        // create <engine> or clean it
        QString tmpPath = output + '/' + engineName;
        if ( dir.exists(tmpPath) ) {
            clearDirectory(tmpPath);
        } else {
            dir.mkdir(tmpPath);
        }

        // create *.baseline or clean it
        tmpPath = output + '/' + engineName + ".baseline";
        if ( dir.exists(tmpPath) ) {
            clearDirectory(tmpPath);
        } else {
            dir.mkdir(tmpPath);
        }

        // create *.diff or clean it
        tmpPath = output + '/' + engineName + ".diff";
        if ( dir.exists(tmpPath) ) {
            clearDirectory(tmpPath);
        } else {
            dir.mkdir(tmpPath);
        }

        // create *.failed or clean it
        tmpPath = output + '/' + engineName + ".failed";
        if ( dir.exists(tmpPath) ) {
            clearDirectory(tmpPath);
        } else {
            dir.mkdir(tmpPath);
        }
    }

    qDebug() << "\t(I) Created on local machine:" << output;
}



/*
 * Setup the remote environment.
 */

void uiLoader::setupFTP()
{
    qDebug( " ========== Setting up FTP environment" );

    // create dirs on ftp server
    ftpMkDir( ftpBaseDir );
    ftpBaseDir += "/" + QLibraryInfo::buildKey();
    ftpMkDir( ftpBaseDir );
    ftpBaseDir += "/" + QString( qVersion() );
    ftpMkDir( ftpBaseDir );

    QString dir = "";
    ftpList(ftpBaseDir + '/' + dir);
    QList<QString> dirListing(lsDirList);

    // create *.failed, *.diff if necessary, else remove the files in it
    // if *.baseline does not exist, memorize it
    QHashIterator<QString, QString> j(enginesToTest);
    while ( j.hasNext() ) {
        j.next();

        QString curDir = QString( j.key() ) + ".failed";
        if ( dirListing.contains( curDir ) ) {
            ftpClearDirectory(ftpBaseDir + "/" + curDir + "/");
        } else {
            ftpMkDir(ftpBaseDir + "/" + curDir + "/");
        }

        curDir = QString( j.key() ) + ".diff";
        if ( dirListing.contains( curDir ) ) {
            ftpClearDirectory(ftpBaseDir + "/" + curDir + "/");
        } else {
            ftpMkDir(ftpBaseDir + "/" + curDir + "/");
        }

        curDir = QString( j.key() ) + ".baseline";
        lsNeedBaseline.clear();
        if ( !dirListing.contains( curDir ) ) {
            ftpMkDir(ftpBaseDir + "/" + curDir + "/");
            lsNeedBaseline << j.key();
        } else {
            qDebug() << "\t(I)" << curDir << "exists on server.";
        }
    }
}



/*
 * Download files listed in fileLisiting from dir pathRemoteDir on sever and save
 * them in pathSaveDir.
 */

void uiLoader::ftpGetFiles(QList<QString>& fileListing, const QString& pathRemoteDir, const QString& pathSaveDir)
{
    QFtp ftp;
    ftp.connectToHost( ftpHost );
    ftp.login( ftpUser, ftpPass );

    if ( !fileListing.empty() ) {
        for ( int i = 0; i < fileListing.size(); ++i ) {
            QFile file( pathSaveDir + "/" +  fileListing.at(i) );

            errorMsg = "could not open file for writing: " + file.fileName();
            QVERIFY2( file.open(QIODevice::WriteOnly), qPrintable(errorMsg) );

            QString ftpFileName = pathRemoteDir + '/' + fileListing.at(i);
            ftp.get( ftpFileName, &file );
            //qDebug() << "\t(I) Got" << file.fileName();
            ftp.list(); //Only there to fill up a slot in the pendingCommands queue.

            while ( ftp.hasPendingCommands() )
                QCoreApplication::instance()->processEvents();

            file.close();
        }
    }

    ftp.close();

    while ( ftp.hasPendingCommands() )
        QCoreApplication::instance()->processEvents();

    fileListing.clear();
}



/*
 * Upload the file filePath to the server and save it there at filePathRemote.
 *
 * HINT: It seems you can't use this function in a loop, to many connections
 *       are established?!
 */

bool uiLoader::ftpUploadFile(const QString& filePathRemote, const QString& filePath)
{
    QFile file(filePath);

    errorMsg = "could not open file: " + filePath;
    QVERIFY3( file.open(QIODevice::ReadOnly), qPrintable(errorMsg), false );

    QByteArray contents = file.readAll();
    file.close();

    qDebug() << "\t(I) Uploading file to" << filePathRemote;

    QFtp ftp;
    ftp.connectToHost( ftpHost );
    ftp.login( ftpUser, ftpPass );

    ftp.put( contents, filePathRemote, QFtp::Binary );

    ftp.close();

    while ( ftp.hasPendingCommands() )
        QCoreApplication::instance()->processEvents();

    return true;
}



/*
 * Enter the dir dir on the server and remove all files (not recursive!)
 */

void uiLoader::ftpClearDirectory(const QString& pathDir)
{
    qDebug() << "\t(I) Clearing directory remote: " << pathDir;

    ftpList(pathDir);
    QList<QString> dirListing(lsDirList);

    QFtp ftp;
    ftp.connectToHost( ftpHost );
    ftp.login( ftpUser, ftpPass );

    for (int i = 0; i < dirListing.size(); ++i) {
        QString file = dirListing.at(i);
        qDebug() << "\t(I) Removing" << pathDir + file;
        ftp.remove(pathDir + file);
    }

    ftp.close();

    while ( ftp.hasPendingCommands() )
        QCoreApplication::instance()->processEvents();
}



/*
 * Get a directory listing from the server in the dir dir.
 * You can access it via lsDirList.
 */

void uiLoader::ftpList(const QString & dir) {
    qDebug() << "\t(I) Getting list of files in dir" << dir;

    lsDirList.clear();

    QFtp ftp;
    QObject::connect( &ftp, SIGNAL( listInfo( const QUrlInfo & ) ), this, SLOT( ftpAddLsEntry(const QUrlInfo & ) ) );
    //QObject::connect( &ftp, SIGNAL( done( bool ) ), this, SLOT( ftpAddLsDone( bool ) ) );

    ftp.connectToHost( ftpHost );
    ftp.login( ftpUser, ftpPass );

    ftp.list( dir );
    ftp.close();

    while ( ftp.hasPendingCommands() )
        QCoreApplication::instance()->processEvents();
}



/*
 * Creates a dir on the ftp server.
 *
 * Hint: If the ftp.mkdir() fails we just assume the dir already exist.
 */

void uiLoader::ftpMkDir( QString pathDir )
{
    QFtp ftp;

    QSignalSpy commandSpy(&ftp, SIGNAL(commandFinished(int, bool)));

    ftp.connectToHost( ftpHost );
    ftp.login( ftpUser, ftpPass );
    const int command = ftp.mkdir( pathDir );
    ftp.close();

    while ( ftp.hasPendingCommands() )
        QCoreApplication::instance()->processEvents();

    // check wheter there was an error or not
    for (int i = 0; i < commandSpy.count(); ++i) {
        if (commandSpy.at(i).at(0) == command) {
            if ( !commandSpy.at(i).at(1).toBool() ) {
                qDebug() << "\t(I) Created at remote machine:" << pathDir;
            } else {
                qDebug() << "\t(I) Could not create on remote machine - probably the dir exists";
            }
        }
    }
}



/*
 * Just a slot, needed for ftpList().
 */

void uiLoader::ftpAddLsEntry( const QUrlInfo &urlInfo )
{
    //Just adding the file to the list
    lsDirList << urlInfo.name();
}

/*
 * Return a list of the test case ui files
 */

QStringList uiLoader::uiFiles() const
{
    QString baselinePath = QDir::currentPath();
    baselinePath += QLatin1String("/baseline");
    QDir dir(baselinePath);
    dir.setFilter(QDir::Files);
    dir.setNameFilters(QStringList(QLatin1String("*.ui")));
    const QFileInfoList list = dir.entryInfoList();
    QStringList rc;
    const QChar slash = QLatin1Char('/');
    foreach (const QFileInfo &fi, list) {
        QString fileAbsolutePath = baselinePath;
        fileAbsolutePath += slash;
        fileAbsolutePath += fi.fileName();
        rc.push_back(fileAbsolutePath);
    }
    return rc;
}
/*
 * The actual method for generating local files that will be compared
 * to the baseline.
 *
 * The external program uiscreenshot/uiscreenshot is called to generate
 * *.png files of *.ui files.
 */

void uiLoader::executeTests()
{
    qDebug(" ========== Executing the tests...[generating pngs from uis]");

    qDebug() << "Current Dir" << QDir::currentPath();

    qDebug() << "\t(I) Using" << pathToProgram;

    QProcess myProcess;
    foreach(const QString &fileAbsolutePath, uiFiles()) {
        qDebug() << "\t(I) Current file:" << fileAbsolutePath;

        QHashIterator<QString, QString> j(enginesToTest);
        while ( j.hasNext() ) {
            j.next();

            QString outputDirectory = output + '/' + j.key();

            QStringList arguments;
            arguments << fileAbsolutePath;
            arguments << outputDirectory;

            myProcess.start(pathToProgram, arguments);

            // took too long?
            errorMsg = "process does not exited normally (QProcess timeout) -  " + pathToProgram;
            QVERIFY2( myProcess.waitForFinished(), qPrintable(errorMsg) );

            qDebug() << "\n" << myProcess.readAllStandardError();

            // check exit code/status
            errorMsg = "process does not exited normally - " + pathToProgram;
            QVERIFY2( myProcess.exitStatus() == QProcess::NormalExit, qPrintable(errorMsg) );
            QVERIFY2( myProcess.exitCode() == EXIT_SUCCESS, qPrintable(errorMsg) );
        }
    }
}

/*
 * Comparing generated files to the baseline.
 */

bool uiLoader::compare()
{
    qDebug( " ========== Now comparing the results to the baseline" );

    QDir dir(output);

    QHashIterator<QString, QString> i(enginesToTest);
    while ( i.hasNext() ) {
        i.next();

        QString engineName = i.key();

        // Perform comparisons between the two directories.
        dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
        dir.setNameFilters( QStringList() << "*.png" );
        dir.cd( engineName + ".baseline" );

        QFileInfoList list = dir.entryInfoList();

        for (int i = 0; i < list.size(); ++i) {
            QFileInfo fileInfo = list.at(i);
            diff(output, engineName, fileInfo.fileName());
        }
    }

    return true;
}




void uiLoader::diff(const QString& basedir, const QString& engine, const QString& fileName)
{
    QString filePathBaseline = basedir + "/" + engine + ".baseline/" + fileName;
    QString filePathGenerated = basedir + "/" + engine + '/' + fileName;

    qDebug() << "\t(I) Comparing" << filePathBaseline;
    qDebug() << "\t(I) Comparing" << filePathGenerated;

    QString filePathDiffImage = basedir + "/" + engine + ".diff/" + fileName;

    if ( QFile::exists(filePathGenerated) ) {
        QString filePathDiffImage = basedir + "/" + engine + ".diff/" + fileName;
        int pixelDiff = imgDiff(filePathBaseline, filePathGenerated, filePathDiffImage);

        if ( pixelDiff <= threshold.toInt() ) {
            qDebug() << "\t(I) TEST OK";
            QVERIFY(true);
        } else {
            qDebug() << "\t(I) TEST FAILED";
            qDebug() << "\t(I)\t...saving baseline in *.failed";

            // local: save in *.failed
            QString filePathFailed = basedir + "/" + engine + ".failed/" + fileName;
            errorMsg = "Could not save " + filePathGenerated + " to " + filePathFailed;
            QVERIFY2( QFile::copy(filePathGenerated, filePathFailed), qPrintable(errorMsg) );

            // remote: save in *.failed
            QString filePathFailedRemote = ftpBaseDir + "/" + engine + ".failed" + "/" + fileName;
            ftpUploadFile(filePathFailedRemote, filePathGenerated);

            errorMsg = "Something broke in the image comparison with  " + filePathDiffImage;
            QVERIFY2( (pixelDiff != -1), qPrintable(errorMsg) );

            // remote: save in *.diff
            QString filePathDiffRemote = ftpBaseDir + "/" + engine + ".diff" + "/" + fileName;
            ftpUploadFile(filePathDiffRemote, filePathDiffImage);
            QFAIL(qPrintable(fileName));
        }

    } else {
        qWarning() << "\t(W) Expected generated file" << filePathGenerated << "does not exist.";
        qWarning() << "\t(W)   ...saving baseline in *.failed";

        // save local
        QString filePathMissing = basedir + '/' + engine + ".failed/" + fileName + "_missing";
        errorMsg = "Could not save " + filePathMissing;
        QVERIFY2( QFile::copy(filePathBaseline, filePathMissing), qPrintable(errorMsg) );

        // save remote
        QString filePathDiffRemote = ftpBaseDir + "/" + engine + ".diff" + "/" + fileName;
        ftpUploadFile(filePathDiffRemote, filePathBaseline);

        errorMsg = filePathGenerated + " was not generated, but baseline for this file exists";
        QVERIFY2(false, qPrintable(errorMsg));
    }

}

/*
 * Execution starts here.
 */

uiLoader::TestResult uiLoader::runAutoTests(QString *errorMessage)
{
    // SVG needs this widget...
    QWidget dummy;

    qDebug() << "Running test on buildkey:" << QLibraryInfo::buildKey() << "  qt version:" << qVersion();
    qDebug() << "Initializing tests...";

    // load config
    const QString configFileName = QHostInfo::localHostName().split(QLatin1Char('.')).first() + QLatin1String(".ini");
    const QFileInfo fi(configFileName);
    if (!fi.isFile() || !fi.isReadable()) {
        *errorMessage = QString::fromLatin1("Config file '%1' does not exist or is not readable.").arg(configFileName);
        return TestNoConfig;
    }

    if (!loadConfig(configFileName, errorMessage))
        return TestConfigError;

    // reset the local environment where the results are stored
    setupLocal();

    // reset the FTP environment where the results are stored
    setupFTP();

    // retrieve the latest test result baseline from the FTP server.
    downloadBaseline();

    // execute tests
    executeTests();

    // upload testresults as new baseline or compare results
    if ( lsNeedBaseline.size() )
        createBaseline();
    else
        compare();

    return TestRunDone;
}

int uiLoader::imgDiff(const QString fileA, const QString fileB, const QString output)
{
//  qDebug() << "Comparing " << fileA << " and " << fileB << " outputting to " << output;
  QImage imageA(fileA);
  QImage imageB(fileB);

  // Invalid images
  if (imageA.isNull() || imageB.isNull())
  {
    qDebug() << "Fatal error: unable to open one or more input images.";
    return false;
  }

  //Choose the largest image size, so that the output can capture the entire diff.
  QSize largestSize = imageA.size();
  QSize otherSize = imageB.size();

  if (largestSize.width() < otherSize.width())
    largestSize.setWidth(otherSize.width());

  if (largestSize.height() < otherSize.height())
    largestSize.setHeight(otherSize.height());

  QImage imageDiff(largestSize, QImage::Format_ARGB32);

  imageA = imageA.convertToFormat(QImage::Format_ARGB32);
  imageB = imageB.convertToFormat(QImage::Format_ARGB32);

  int pixelDiff = 0;

  for (int y = 0; y < imageDiff.height(); ++y)
  {
    for (int x = 0; x < imageDiff.width(); ++x)
    {
      //Are the pixels within range? Else, draw a black pixel in diff.
      if (imageA.valid(x,y) && imageB.valid(x,y))
      {
        //Both images have a pixel at x,y - are they the same? If not, black pixel in diff.
        if (imageA.pixel(x,y) != imageB.pixel(x,y))
        {
          imageDiff.setPixel(x,y,0xff000000);
          pixelDiff++;
        }
        else
          imageDiff.setPixel(x,y,0xffffffff);
      }
      else
      {
        imageDiff.setPixel(x,y,0xff000000);
        pixelDiff++;
      }
    }
  }

  imageDiff.setText("comment", QString::number(pixelDiff));

  if (!imageDiff.save(output, "PNG"))
      pixelDiff = -1;

  return pixelDiff;
}
