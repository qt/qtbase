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


#include <atWrapper.h>
#include <datagenerator/datagenerator.h>

#include <QString>
#include <QHash>
#include <QFile>
#include <QFtp>
#include <QObject>
#include <QHostInfo>
#include <QWidget>
#include <QImage>
#include <QtTest/QSignalSpy>
#include <QLibraryInfo>

static const char *ArthurDir = "../../arthur";

#include <string.h>

atWrapper::atWrapper()
{

 //   initTests();

}

bool atWrapper::initTests(bool *haveBaseline)
{
    qDebug() << "Running test on buildkey:" << QLibraryInfo::buildKey() << "  qt version:" << qVersion();

    qDebug( "Initializing tests..." );

    if (!loadConfig( QHostInfo::localHostName().split( "." ).first() + ".ini" ))
        return false;

    //Reset the FTP environment where the results are stored
    *haveBaseline = setupFTP();

    // Retrieve the latest test result baseline from the FTP server.
    downloadBaseline();
    return true;
}

void atWrapper::downloadBaseline()
{

    qDebug() << "Now downloading baseline...";

    QFtp ftp;

    QObject::connect( &ftp, SIGNAL(listInfo(QUrlInfo)), this, SLOT(ftpMgetAddToList(QUrlInfo)) );

    //Making sure that the needed local directories exist.

    QHashIterator<QString, QString> j(enginesToTest);

    while ( j.hasNext() )
    {
        j.next();

        QDir dir( output );

        if ( !dir.cd( j.key() + ".baseline" ) )
            dir.mkdir( j.key() + ".baseline" );

    }

    //FTP to the host specified in the config file, and retrieve the test result baseline.
    ftp.connectToHost( ftpHost );
    ftp.login( ftpUser, ftpPass );

    ftp.cd( ftpBaseDir );

    QHashIterator<QString, QString> i(enginesToTest);
    while ( i.hasNext() )
    {
        i.next();
        mgetDirList.clear();
        mgetDirList << i.key() + ".baseline";
        ftp.cd( i.key() + ".baseline" );
        ftp.list();
        ftp.cd( ".." );

        while ( ftp.hasPendingCommands() )
            QCoreApplication::instance()->processEvents();

        ftpMgetDone( true );
    }

    ftp.close();
    ftp.close();

    while ( ftp.hasPendingCommands() )
        QCoreApplication::instance()->processEvents();

}

void atWrapper::ftpMgetAddToList( const QUrlInfo &urlInfo )
{
    //Simply adding to the list of files to download.
    mgetDirList << urlInfo.name();

}

void atWrapper::ftpMgetDone( bool error)
{
    Q_UNUSED( error );

    //Downloading the files listed in mgetDirList...
    QFtp ftp;
    ftp.connectToHost( ftpHost );
    ftp.login( ftpUser, ftpPass );

    QFile* file;

    if ( mgetDirList.size() > 1 )
        for ( int i = 1; i < mgetDirList.size(); ++i )
        {
            file = new QFile( QString( output ) + "/" + mgetDirList.at( 0 ) + "/" + mgetDirList.at( i ) );
            if (file->open(QIODevice::WriteOnly)) {
                ftp.get( ftpBaseDir + "/" + mgetDirList.at( 0 ) + "/" + mgetDirList.at( i ), file );
                ftp.list(); //Only there to fill up a slot in the pendingCommands queue.
                while ( ftp.hasPendingCommands() )
                    QCoreApplication::instance()->processEvents();
                file->close();
            } else {
                qDebug() << "Couldn't open file for writing: " << file->fileName();
            }
        }


    while ( ftp.hasPendingCommands() )
        QCoreApplication::instance()->processEvents();
}

void atWrapper::uploadFailed( QString dir, QString filename, QByteArray filedata )
{
    //Upload a failed test case image to the FTP server.
    QFtp ftp;
    ftp.connectToHost( ftpHost );
    ftp.login( ftpUser, ftpPass );

    ftp.cd( ftpBaseDir );
    ftp.cd( dir );

    ftp.put( filedata, filename, QFtp::Binary );

    ftp.close();

    while ( ftp.hasPendingCommands() )
        QCoreApplication::instance()->processEvents();
}

// returns false if no baseline exists
bool atWrapper::setupFTP()
{
    qDebug( "Setting up FTP environment" );

    QString dir = "";
    ftpMkDir( ftpBaseDir );

    ftpBaseDir += "/" + QLibraryInfo::buildKey();

    ftpMkDir( ftpBaseDir );

    ftpBaseDir += "/" + QString( qVersion() );

    ftpMkDir( ftpBaseDir );

    QHashIterator<QString, QString> i(enginesToTest);
    QHashIterator<QString, QString> j(enginesToTest);

    bool haveBaseline = true;
    //Creating the baseline directories for each engine
    while ( i.hasNext() )
    {
        i.next();
        //qDebug() << "Creating dir with key:" << i.key();
        ftpMkDir( ftpBaseDir + "/" +  QString( i.key() ) + ".failed" );
        ftpMkDir( ftpBaseDir + "/" +  QString( i.key() ) + ".diff" );
        if (!ftpMkDir( ftpBaseDir + "/" + QString( i.key() ) + ".baseline" ))
            haveBaseline = false;
    }


    QFtp ftp;
    ftp.connectToHost( ftpHost );
    ftp.login( ftpUser, ftpPass );

    ftp.cd( ftpBaseDir );
    //Deleting previous failed directory and all the files in it, then recreating it.
    while ( j.hasNext() )
    {
        j.next();
        rmDirList.clear();
        rmDirList << ftpBaseDir + "/" + j.key() + ".failed" + "/";
        ftpRmDir( j.key() + ".failed" );
        ftp.rmdir( j.key() + ".failed" );
        ftp.mkdir( j.key() + ".failed" );
        ftp.list();

        while ( ftp.hasPendingCommands() )
            QCoreApplication::instance()->processEvents();

        rmDirList.clear();
        rmDirList << ftpBaseDir + "/" + j.key() + ".diff" + "/";
        ftpRmDir( j.key() + ".diff" );
        ftp.rmdir( j.key() + ".diff" );
        ftp.mkdir( j.key() + ".diff" );
        ftp.list();

        while ( ftp.hasPendingCommands() )
            QCoreApplication::instance()->processEvents();

    }

    ftp.close();

    while ( ftp.hasPendingCommands() )
        QCoreApplication::instance()->processEvents();

    return haveBaseline;
}

void atWrapper::ftpRmDir( QString dir )
{
    //Hack to remove a populated directory. (caveat: containing only files and empty dirs, not recursive!)
    qDebug() << "Now removing directory: " << dir;
    QFtp ftp;
    QObject::connect( &ftp, SIGNAL(listInfo(QUrlInfo)), this, SLOT(ftpRmDirAddToList(QUrlInfo)) );
    QObject::connect( &ftp, SIGNAL(done(bool)), this, SLOT(ftpRmDirDone(bool)) );

    ftp.connectToHost( ftpHost );
    ftp.login( ftpUser, ftpPass );

    ftp.list( ftpBaseDir + "/" +  dir );
    ftp.close();
    ftp.close();

    while ( ftp.hasPendingCommands() )
                QCoreApplication::instance()->processEvents();
}

void atWrapper::ftpRmDirDone( bool error )
{
    //Deleting each file in the directory listning, rmDirList.
    Q_UNUSED( error );

    QFtp ftp;
    ftp.connectToHost( ftpHost );
    ftp.login( ftpUser, ftpPass );

    if ( rmDirList.size() > 1 )
        for (int i = 1; i < rmDirList.size(); ++i)
            ftp.remove( rmDirList.at(0) + rmDirList.at( i ) );

    ftp.close();

    while ( ftp.hasPendingCommands() )
        QCoreApplication::instance()->processEvents();
}

// returns false if the directory already exists
bool atWrapper::ftpMkDir( QString dir )
{
    //Simply used to avoid QFTP from bailing out and loosing a queue of commands.
    // IE: conveniance.
    QFtp ftp;

    QSignalSpy commandSpy(&ftp, SIGNAL(commandFinished(int,bool)));

    ftp.connectToHost( ftpHost );
    ftp.login( ftpUser, ftpPass );
    const int command = ftp.mkdir( dir );
    ftp.close();

    while ( ftp.hasPendingCommands() )
        QCoreApplication::instance()->processEvents();

    for (int i = 0; i < commandSpy.count(); ++i)
        if (commandSpy.at(i).at(0) == command)
            return commandSpy.at(i).at(1).toBool();

    return false;
}


void atWrapper::ftpRmDirAddToList( const QUrlInfo &urlInfo )
{
    //Just adding the file to the list for deletion
    rmDirList << urlInfo.name();
}


bool atWrapper::executeTests()
{
    qDebug("Executing the tests...");

    QHashIterator<QString, QString> i(enginesToTest);

    DataGenerator generator;

    //Running datagenerator against all the frameworks specified in the config file.
    while ( i.hasNext() )
    {

        i.next();

        qDebug( "Now testing: " + i.key().toLatin1() );

        char* params[13];
        //./bin/datagenerator  -framework data/framework.ini  -engine OpenGL -suite 1.1 -output outtest


        QByteArray eng = i.key().toLatin1();
        QByteArray fwk = framework.toLatin1();
        QByteArray sut = suite.toLatin1();
        QByteArray out = output.toLatin1();
        QByteArray siz = size.toLatin1();
        QByteArray fill = fillColor.toLatin1();

        params[1] = "-framework";
        params[2] = fwk.data();
        params[3] = "-engine";
        params[4] = eng.data();
        params[5] = "-suite";
        params[6] = sut.data();
        params[7] = "-output";
        params[8] = out.data();
        params[9] = "-size";
        params[10] = siz.data();
        params[11] = "-fill";
        params[12] = fill.data();

        generator.run( 13, params );
    }

    return true;
}

void atWrapper::createBaseline()
{
    qDebug( "Now uploading a baseline of only the latest test values" );

    QHashIterator<QString, QString> i(enginesToTest);

    QDir dir( output );
    QFtp ftp;
    ftp.connectToHost( ftpHost );
    ftp.login( ftpUser, ftpPass );
    ftp.cd( ftpBaseDir );
    //Upload all the latest test results to the FTP server's baseline directory.
    while ( i.hasNext() )
    {

        i.next();
        dir.cd( i.key() );
        ftp.cd( i.key() + ".baseline" );
        dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
        dir.setNameFilters( QStringList() << "*.png" );
        QFileInfoList list = dir.entryInfoList();
        dir.cd( ".." );
        for (int n = 0; n < list.size(); n++)
        {
            QFileInfo fileInfo = list.at( n );
            QFile file( QString( output ) + "/" + i.key() + "/" + fileInfo.fileName() );
            file.open( QIODevice::ReadOnly );
            QByteArray fileData = file.readAll();
            //qDebug() << "Sending up:" << fileInfo.fileName() << "with file size" << fileData.size();
            file.close();
            ftp.put( fileData, fileInfo.fileName(), QFtp::Binary );
        }

        ftp.cd( ".." );
    }

    ftp.close();

    while ( ftp.hasPendingCommands() )
        QCoreApplication::instance()->processEvents();
}

bool atWrapper::compare()
{
    qDebug( "Now comparing the results to the baseline" );

    QHashIterator<QString, QString> i(enginesToTest);

    while ( i.hasNext() )
    {
        i.next();

        compareDirs( output , i.key() );

    }

    return true;
}

void atWrapper::compareDirs( QString basedir, QString target )
{

    QDir dir( basedir );

    /* The following should be redundant now.

    if ( !dir.cd( target + ".failed" ) )
        dir.mkdir( target + ".failed" );
    else
        dir.cdUp();

    */

     if ( !dir.cd( target + ".diff" ) )
         dir.mkdir( target + ".diff" );
     else
         dir.cdUp();



    //Perform comparisons between the two directories.

    dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dir.setNameFilters( QStringList() << "*.png" );
    dir.cd( target + ".baseline" );
    QFileInfoList list = dir.entryInfoList();

    for (int i = 0; i < list.size(); ++i)
    {
        QFileInfo fileInfo = list.at(i);
        diff ( basedir, target, fileInfo.fileName() );
    }
}

bool atWrapper::diff( QString basedir, QString dir, QString target )
{
    //Comparing the two specified files, and then uploading them to
    //the ftp server if they differ

    basedir += "/" + dir;
    QString one = basedir + ".baseline/" + target;
    QString two = basedir + "/" + target;

    QFile file( one );

    file.open( QIODevice::ReadOnly );
    QByteArray contentsOfOne = file.readAll();
    file.close();

    file.setFileName( two );

    file.open( QIODevice::ReadOnly );
    QByteArray contentsOfTwo = file.readAll();
    file.close();

    if ( contentsOfTwo.size() == 0 )
    {
        qDebug() << "No test result found for baseline: " << one;
        file.setFileName( one );
        file.open( QIODevice::ReadOnly );
        file.copy( basedir + ".failed/" + target + "_missing"  );
        uploadFailed( dir + ".failed", target + "_missing", contentsOfTwo );
        return false;
    }


    if ( ( memcmp( contentsOfOne, contentsOfTwo, contentsOfOne.size() ) ) == 0 )
        return true;
    else
    {
        qDebug() << "Sorry, the result did not match: " << one;
        file.setFileName( two );
        file.open( QIODevice::ReadOnly );
        file.copy( basedir + ".failed/" + target  );
        file.close();
        uploadFailed( dir + ".failed", target, contentsOfTwo );
        uploadDiff( basedir, dir, target );
        return false;
    }
}

void atWrapper::uploadDiff( QString basedir, QString dir, QString filename )
{

    qDebug() << basedir;
    QImage im1( basedir + ".baseline/" + filename );
    QImage im2( basedir + "/" + filename );

    QImage im3(im1.size(), QImage::Format_ARGB32);

    im1 = im1.convertToFormat(QImage::Format_ARGB32);
    im2 = im2.convertToFormat(QImage::Format_ARGB32);

    for ( int y=0; y<im1.height(); ++y )
    {
        uint *s = (uint *) im1.scanLine(y);
        uint *d = (uint *) im2.scanLine(y);
        uint *w = (uint *) im3.scanLine(y);

        for ( int x=0; x<im1.width(); ++x )
        {
            if (*s != *d)
                *w = 0xff000000;
            else
                *w = 0xffffffff;
        w++;
        s++;
        d++;
        }
    }

    im3.save( basedir + ".diff/" + filename ,"PNG");

    QFile file( basedir + ".diff/" + filename );
    file.open( QIODevice::ReadOnly );
    QByteArray contents = file.readAll();
    file.close();

    uploadFailed( dir + ".diff", filename, contents );

}

bool atWrapper::loadConfig( QString path )
{
    qDebug() << "Loading config file from ... " << path;
    configPath = path;
    //If there is no config file, don't proceed;
    if ( !QFile::exists( path ) )
    {
        return false;
    }


    QSettings settings( path, QSettings::IniFormat, this );


    //FIXME: Switch to QStringList or something, hash is not needed!
    int numEngines = settings.beginReadArray("engines");

    for ( int i = 0; i < numEngines; ++i )
    {
        settings.setArrayIndex(i);
        enginesToTest.insert( settings.value( "engine" ).toString(), "Info here please :p" );
    }

    settings.endArray();

    framework = QString(ArthurDir) + QDir::separator() + settings.value( "framework" ).toString();
    suite = settings.value( "suite" ).toString();
    output = settings.value( "output" ).toString();
    size = settings.value( "size", "480,360" ).toString();
    fillColor = settings.value( "fill", "white" ).toString();
    ftpUser = settings.value( "ftpUser" ).toString();
    ftpPass = settings.value( "ftpPass" ).toString();
    ftpHost = settings.value( "ftpHost" ).toString();
    ftpBaseDir = settings.value( "ftpBaseDir" ).toString();


    QDir::current().mkdir( output );

    output += "/" + QLibraryInfo::buildKey();

    QDir::current().mkdir( output );

    output += "/" + QString( qVersion() );

    QDir::current().mkdir( output );


    ftpBaseDir += "/" + QHostInfo::localHostName().split( "." ).first();


/*
    framework = "data/framework.ini";
    suite = "1.1";
    output = "testresults";
    ftpUser = "anonymous";
    ftpPass = "anonymouspass";
    ftpHost = "kramer.troll.no";
    ftpBaseDir = "/arthurtest";
*/
    return true;
}

bool atWrapper::runAutoTests()
{
    //SVG needs this widget...
    QWidget dummy;

    bool haveBaseline = false;

    if (!initTests(&haveBaseline))
        return false;
    executeTests();

    if ( !haveBaseline )
    {
        qDebug( " First run! Creating baseline..." );
        createBaseline();
    }
    else
    {
        qDebug( " Comparing results..." );
        compare();
    }
    return true;
}
