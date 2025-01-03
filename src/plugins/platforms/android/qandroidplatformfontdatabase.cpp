// Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QDir>

#include "qandroidplatformfontdatabase.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QString QAndroidPlatformFontDatabase::fontDir() const
{
    return "/system/fonts"_L1;
}

QFont QAndroidPlatformFontDatabase::defaultFont() const
{
    return QFont("Roboto"_L1);
}

void QAndroidPlatformFontDatabase::populateFontDatabase()
{
    QString fontpath = fontDir();
    QDir dir(fontpath);

    if (Q_UNLIKELY(!dir.exists())) {
        qFatal("QFontDatabase: Cannot find font directory %s - is Qt installed correctly?",
               qPrintable(fontpath));
    }

    QStringList nameFilters;
    nameFilters << "*.ttf"_L1
                << "*.otf"_L1
                << "*.ttc"_L1;

    const auto entries = dir.entryInfoList(nameFilters, QDir::Files);
    for (const QFileInfo &fi : entries) {
        const QByteArray file = QFile::encodeName(fi.absoluteFilePath());
        QFreeTypeFontDatabase::addTTFile(QByteArray(), file);
    }
}

QStringList QAndroidPlatformFontDatabase::fallbacksForFamily(const QString &family,
                                                             QFont::Style style,
                                                             QFont::StyleHint styleHint,
                                                             QChar::Script script) const
{
    QStringList result;
    if (styleHint == QFont::Monospace || styleHint == QFont::Courier)
        result.append(QString(qgetenv("QT_ANDROID_FONTS_MONOSPACE")).split(u';'));
    else if (styleHint == QFont::Serif)
        result.append(QString(qgetenv("QT_ANDROID_FONTS_SERIF")).split(u';'));
    else
        result.append(QString(qgetenv("QT_ANDROID_FONTS")).split(u';'));
    result.append(QFreeTypeFontDatabase::fallbacksForFamily(family, style, styleHint, script));

    return result;
}

QT_END_NAMESPACE
