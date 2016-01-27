/****************************************************************************
**
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include <QDir>

#include "qandroidplatformfontdatabase.h"

QT_BEGIN_NAMESPACE

QString QAndroidPlatformFontDatabase::fontDir() const
{
    return QLatin1String("/system/fonts");
}

void QAndroidPlatformFontDatabase::populateFontDatabase()
{
    QString fontpath = fontDir();
    QDir dir(fontpath);

    if (!dir.exists()) {
        qFatal("QFontDatabase: Cannot find font directory %s - is Qt installed correctly?",
               qPrintable(fontpath));
    }

    QStringList nameFilters;
    nameFilters << QLatin1String("*.ttf")
                << QLatin1String("*.otf");

    foreach (const QFileInfo &fi, dir.entryInfoList(nameFilters, QDir::Files)) {
        const QByteArray file = QFile::encodeName(fi.absoluteFilePath());
        QBasicFontDatabase::addTTFile(QByteArray(), file);
    }
}

QStringList QAndroidPlatformFontDatabase::fallbacksForFamily(const QString &family,
                                                             QFont::Style style,
                                                             QFont::StyleHint styleHint,
                                                             QChar::Script script) const
{
    QStringList result;
    if (styleHint == QFont::Monospace || styleHint == QFont::Courier)
        result.append(QString(qgetenv("QT_ANDROID_FONTS_MONOSPACE")).split(QLatin1Char(';')));
    else if (styleHint == QFont::Serif)
        result.append(QString(qgetenv("QT_ANDROID_FONTS_SERIF")).split(QLatin1Char(';')));
    else
        result.append(QString(qgetenv("QT_ANDROID_FONTS")).split(QLatin1Char(';')));
    result.append(QBasicFontDatabase::fallbacksForFamily(family, style, styleHint, script));

    return result;
}

QT_END_NAMESPACE
