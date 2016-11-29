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

#include <qmap.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qlist.h>
#include <qbuffer.h>
#include <qtextstream.h>
#include <qdir.h>

QT_BEGIN_NAMESPACE

class Configure
{
public:
    Configure( int& argc, char** argv );
    ~Configure();

    void parseCmdLine();

    void generateQConfigCpp();
    void buildQmake();

    void prepareConfigureInput();
    void configure();

    void generateHeaders();
    void generateQDevicePri();
    void prepareConfigTests();

    bool showLicense(QString licenseFile);
    void readLicense();

    bool isDone();
    bool isOk();

    int platform() const;
    QString platformName() const;

private:
    int verbose;

    // Our variable dictionaries
    QMap<QString,QString> dictionary;
    QStringList configCmdLine;

    QString outputLine;

    QTextStream outStream;
    QString sourcePath, buildPath;
    QString sourcePathMangled, buildPathMangled;
    QDir sourceDir, buildDir;

    QString confStrOffsets[2];
    QString confStrings[2];
    int confStringOff;

    void addConfStr(int group, const QString &val);
    QString formatPath(const QString &path);

    bool reloadCmdLine(int idx);
    void saveCmdLine();

    void applySpecSpecifics();
};

class FileWriter : public QTextStream
{
public:
    FileWriter(const QString &name);
    bool flush();
private:
    QString m_name;
    QBuffer m_buffer;
};

QT_END_NAMESPACE
