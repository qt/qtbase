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

#include "tools.h"
#include "environment.h"

#include <qdir.h>
#include <qfile.h>
#include <qbytearray.h>
#include <qstringlist.h>

#include <iostream>

std::ostream &operator<<(std::ostream &s, const QString &val);
using namespace std;

void Tools::checkLicense(QMap<QString,QString> &dictionary,
                         const QString &sourcePath, const QString &buildPath)
{
    dictionary["LICHECK"] = "licheck.exe";

    const QString licenseChecker =
        QDir::toNativeSeparators(sourcePath + "/bin/licheck.exe");

    if (QFile::exists(licenseChecker)) {
        const QString qMakeSpec =
            QDir::toNativeSeparators(dictionary.value("QMAKESPEC"));
        const QString xQMakeSpec =
            QDir::toNativeSeparators(dictionary.value("XQMAKESPEC"));

        QString command = QString("%1 %2 %3 %4 %5 %6")
            .arg(licenseChecker,
                 dictionary.value("LICENSE_CONFIRMED", "no"),
                 QDir::toNativeSeparators(sourcePath),
                 QDir::toNativeSeparators(buildPath),
                 qMakeSpec, xQMakeSpec);

        int returnValue = 0;
        QString licheckOutput = Environment::execute(command, &returnValue);

        if (returnValue) {
            dictionary["DONE"] = "error";
        } else {
            foreach (const QString &var, licheckOutput.split('\n'))
                dictionary[var.section('=', 0, 0).toUpper()] = var.section('=', 1, 1);
            dictionary["LICENSE_CONFIRMED"] = "yes";
        }
    } else {
        cout << endl << "Error: Could not find licheck.exe" << endl
             << "Try re-installing." << endl << endl;
        dictionary["DONE"] = "error";
    }
}

