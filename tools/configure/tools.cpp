/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include "tools.h"

#include <qdir.h>
#include <qfile.h>
#include <qbytearray.h>


// std stuff ------------------------------------
#include <iostream>
#include <windows.h>
#include <conio.h>
#define NUMBER_OF_PARTS 7

std::ostream &operator<<(std::ostream &s, const QString &val); // defined in configureapp.cpp
using namespace std;

void Tools::checkLicense(QMap<QString,QString> &dictionary, QMap<QString,QString> &licenseInfo,
                         const QString &path)
{
    QString tpLicense = dictionary["QT_SOURCE_TREE"] + "/LICENSE.PREVIEW.OPENSOURCE";
    if (QFile::exists(tpLicense)) {
        dictionary["EDITION"] = "Preview";
        dictionary["LICENSE FILE"] = tpLicense;
        dictionary["QT_EDITION"] = "QT_EDITION_OPENSOURCE";
        return; // No license key checking in Tech Preview
    }
    tpLicense = dictionary["QT_SOURCE_TREE"] + "/LICENSE.PREVIEW.COMMERCIAL";
    if (QFile::exists(tpLicense)) {
        dictionary["EDITION"] = "Preview";
        dictionary["LICENSE FILE"] = tpLicense;
        dictionary["QT_EDITION"] = "QT_EDITION_DESKTOP";
        return; // No license key checking in Tech Preview
    }

    // Read in the license file
    QFile licenseFile(path);
    if( !path.isEmpty() && licenseFile.open( QFile::ReadOnly ) ) {
        cout << "Reading license file in....." << qPrintable(path) << endl;

        QString buffer = licenseFile.readLine(1024);
        while (!buffer.isEmpty()) {
            if( buffer[ 0 ] != '#' ) {
                QStringList components = buffer.split( '=' );
                if ( components.size() >= 2 ) {
                    QStringList::Iterator it = components.begin();
                    QString key = (*it++).trimmed().remove('"').toUpper();
                    QString value = (*it++).trimmed().remove('"');
                    licenseInfo[ key ] = value;
                }
            }
            // read next line
            buffer = licenseFile.readLine(1024);
        }
        licenseFile.close();
    } else {
        cout << "License file not found in " << QDir::homePath() << endl;
        cout << "Please put the Qt license file, '.qt-license' in your home "
             << "directory and run configure again.";
        dictionary["DONE"] = "error";
        return;
    }

    // Verify license info...
    QString licenseKey = licenseInfo["LICENSEKEYEXT"];
    QByteArray clicenseKey = licenseKey.toLatin1();
    //We check the license
    static const char * const SEP = "-";
    char *licenseParts[NUMBER_OF_PARTS];
    int partNumber = 0;
    for (char *part = strtok(clicenseKey.data(), SEP); part != 0; part = strtok(0, SEP))
        licenseParts[partNumber++] = part;
    if (partNumber < (NUMBER_OF_PARTS-1)) {
        dictionary["DONE"] = "error";
        cout << "License file does not contain proper license key." <<partNumber<< endl;
        return;
    }

    char products = licenseParts[0][0];
    char* platforms = licenseParts[1];
    char* licenseSchema = licenseParts[2];
    char licenseFeatures = licenseParts[3][0];

    // Determine edition ---------------------------------------------------------------------------
    QString licenseType;
    if (strcmp(licenseSchema,"F4M") == 0) {
        licenseType = "Commercial";
        if (products == 'F') {
            dictionary["EDITION"] = "Universal";
            dictionary["QT_EDITION"] = "QT_EDITION_UNIVERSAL";
        } else if (products == 'B') {
            dictionary["EDITION"] = "FullFramework";
            dictionary["QT_EDITION"] = "QT_EDITION_DESKTOP";
        } else {
            dictionary["EDITION"] = "GUIFramework";
            dictionary["QT_EDITION"] = "QT_EDITION_DESKTOPLIGHT";
        }
    } else if (strcmp(licenseSchema,"Z4M") == 0 || strcmp(licenseSchema,"R4M") == 0 || strcmp(licenseSchema,"Q4M") == 0) {
        if (products == 'B') {
            dictionary["EDITION"] = "Evaluation";
            dictionary["QT_EDITION"] = "QT_EDITION_EVALUATION";
            licenseType = "Evaluation";
        }
    }

    if (platforms[2] == 'L') {
        static const char src[] = "8NPQRTZ";
        static const char dst[] = "UCWX9M7";
        const char *p = strchr(src, platforms[1]);
        platforms[1] = dst[p - src];
    }

#define PL(a,b) (int(a)+int(b)*256)
    int platformCode = PL(platforms[0],platforms[1]);
    switch (platformCode) {
    case PL('X','9'):
    case PL('X','C'):
    case PL('X','U'):
    case PL('X','W'):
	case PL('X','M'): // old license key
        dictionary["LICENSE_EXTENSION"] = "-ALLOS";
        break;

    case PL('6', 'M'):
    case PL('8', 'M'):
	case PL('K', 'M'): // old license key
    case PL('N', '7'):
    case PL('N', '9'):
    case PL('N', 'X'):
    case PL('S', '9'):
    case PL('S', 'C'):
    case PL('S', 'U'):
    case PL('S', 'W'):
        dictionary["LICENSE_EXTENSION"] = "-EMBEDDED";
        if (dictionary["PLATFORM NAME"].contains("Windows CE")
            && platformCode != PL('6', 'M') && platformCode != PL('S', '9')
            && platformCode != PL('S', 'C') && platformCode != PL('S', 'U')
            && platformCode != PL('S', 'W') && platformCode != PL('K', 'M')) {
            dictionary["DONE"] = "error";
        }
        break;
    case PL('R', 'M'):
    case PL('F', 'M'):
        dictionary["LICENSE_EXTENSION"] = "-DESKTOP";
        if (!dictionary["PLATFORM NAME"].endsWith("Windows")) {
            dictionary["DONE"] = "error";
        }
        break;
    default:
        dictionary["DONE"] = "error";
        break;
    }
#undef PL

    if (dictionary.value("DONE") == "error") {
        cout << "You are not licensed for the " << dictionary["PLATFORM NAME"] << " platform." << endl << endl;
        cout << "Please use the contact form at http://qt.digia.com/contact-us to upgrade your license" << endl;
        cout << "to include the " << dictionary["PLATFORM NAME"] << " platform, or install the" << endl;
        cout << "Qt Open Source Edition if you intend to develop free software." << endl;
        return;
    }

    // Override for evaluation licenses
    if (dictionary["EDITION"] == "Evaluation")
        dictionary["LICENSE_EXTENSION"] = "-EVALUATION";

    if (QFile::exists(dictionary["QT_SOURCE_TREE"] + "/.LICENSE")) {
        // Generic, no-suffix license
        dictionary["LICENSE_EXTENSION"].clear();
    } else if (dictionary["LICENSE_EXTENSION"].isEmpty()) {
        cout << "License file does not contain proper license key." << endl;
        dictionary["DONE"] = "error";
    }
    if (licenseType.isEmpty()
        || dictionary["EDITION"].isEmpty()
        || dictionary["QT_EDITION"].isEmpty()) {
        cout << "License file does not contain proper license key." << endl;
        dictionary["DONE"] = "error";
        return;
    }

    // copy one of .LICENSE-*(-US) to LICENSE
    QString toLicenseFile   = dictionary["QT_SOURCE_TREE"] + "/LICENSE";
    QString fromLicenseFile = dictionary["QT_SOURCE_TREE"] + "/.LICENSE" + dictionary["LICENSE_EXTENSION"];
    if (licenseFeatures == 'B' || licenseFeatures == 'G'
        || licenseFeatures == 'L' || licenseFeatures == 'Y')
        fromLicenseFile += "-US";

    if (!CopyFile((wchar_t*)QDir::toNativeSeparators(fromLicenseFile).utf16(),
        (wchar_t*)QDir::toNativeSeparators(toLicenseFile).utf16(), false)) {
        cout << "Failed to copy license file (" << fromLicenseFile << ")";
        dictionary["DONE"] = "error";
        return;
    }
    dictionary["LICENSE FILE"] = toLicenseFile;
}

