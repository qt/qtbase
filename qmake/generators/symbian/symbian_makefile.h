/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the qmake application of the Qt Toolkit.
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

#ifndef SYMBIAN_MAKEFILE_H
#define SYMBIAN_MAKEFILE_H

#include "symbiancommon.h"

// This allows us to reuse the code for both win32 and unix makefile generators.
template <class T>
class SymbianMakefileTemplate : public T, public SymbianCommonGenerator
{
public:
    SymbianMakefileTemplate() : SymbianCommonGenerator(this) {}

    void init()
    {
        T::init();
        SymbianCommonGenerator::init();
    }

    bool writeMakefile(QTextStream &t)
    {
        QString numberOfIcons;
        QString iconFile;
        QMap<QString, QStringList> userRssRules;
        readRssRules(numberOfIcons, iconFile, userRssRules);

        // Generate pkg files if there are any actual files to deploy
        bool generatePkg = false;

        if (targetType == TypeExe) {
            generatePkg = true;
        } else {
            const QStringList deployments = this->project->values("DEPLOYMENT");
            for (int i = 0; i < deployments.count(); ++i) {
                // ### Qt 5: remove .sources, inconsistent with INSTALLS
                if (!this->project->values(deployments.at(i) + ".sources").isEmpty() ||
                    !this->project->values(deployments.at(i) + ".files").isEmpty()) {
                    generatePkg = true;
                    break;
                }
            }
        }

        SymbianLocalizationList symbianLocalizationList;
        parseTsFiles(&symbianLocalizationList);

        if (generatePkg) {
            generatePkgFile(iconFile, false, symbianLocalizationList);
        }

        if (targetType == TypeExe) {
            if (!this->project->values("CONFIG").contains("no_icon", Qt::CaseInsensitive)) {
                writeRegRssFile(userRssRules);
                writeRssFile(numberOfIcons, iconFile);
                writeLocFile(symbianLocalizationList);
            }
        }

        writeCustomDefFile();

        return T::writeMakefile(t);
    }
};

#endif // SYMBIAN_MAKEFILE_H
