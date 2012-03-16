/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWINDOWSFONTDATABASEFT_H
#define QWINDOWSFONTDATABASEFT_H

#include <QtPlatformSupport/private/qbasicfontdatabase_p.h>
#include <QtCore/QSharedPointer>
#include "qtwindows_additional.h"

QT_BEGIN_NAMESPACE

class QWindowsFontDatabaseFT : public QBasicFontDatabase
{
public:
    void populateFontDatabase();
    QFontEngine *fontEngine(const QFontDef &fontDef, QUnicodeTables::Script script, void *handle);
    QFontEngine *fontEngine(const QByteArray &fontData, qreal pixelSize, QFont::HintingPreference hintingPreference);

    QStringList fallbacksForFamily(const QString family, const QFont::Style &style, const QFont::StyleHint &styleHint, const QUnicodeTables::Script &script) const;
    QStringList addApplicationFont(const QByteArray &fontData, const QString &fileName);

    virtual QString fontDir() const;
    virtual QFont defaultFont() const;

    static HFONT systemFont();
    static QFont LOGFONT_to_QFont(const LOGFONT& lf, int verticalDPI = 0);

private:
    void populate(const QString &family = QString());

    QSet<QString> m_families;
};

QT_END_NAMESPACE

#endif // QWINDOWSFONTDATABASEFT_H
